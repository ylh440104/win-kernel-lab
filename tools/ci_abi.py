import argparse
import base64
import hashlib
import io
import json
import pathlib
import re
import sys
import time
import urllib.error
import urllib.request
import urllib.parse
import zipfile


class DownloadRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, req, fp, code, msg, headers, newurl):
        redirected = super().redirect_request(req, fp, code, msg, headers, newurl)
        if redirected is not None and urllib.parse.urlsplit(req.full_url).netloc != urllib.parse.urlsplit(newurl).netloc:
            redirected.remove_header('Authorization')
        return redirected


HTTP = urllib.request.build_opener(DownloadRedirect())

ROOT = pathlib.Path('/tmp/wkl')
OUT = pathlib.Path('/sdcard/Download/lxcore/winkernel/sec1717-abi')
TOKEN = pathlib.Path('/sdcard/Download/lxcore/.gh').read_text().strip()
API = 'https://api.github.com/repos/ylh440104/win-kernel-lab/'


def request(path, method='GET', data=None, binary=False):
    url = path if path.startswith('https://') else API + path
    headers = {'Authorization': 'Bearer ' + TOKEN, 'Accept': 'application/vnd.github+json', 'User-Agent': 'win-kernel-lab-ci'}
    body = None if data is None else json.dumps(data).encode()
    if body is not None:
        headers['Content-Type'] = 'application/json'
    for attempt in range(3 if method == 'GET' else 1):
        try:
            req = urllib.request.Request(url, data=body, headers=headers, method=method)
            with HTTP.open(req, timeout=25) as response:
                raw = response.read()
                expected = response.headers.get('Content-Length')
                if expected is not None and len(raw) != int(expected):
                    raise ValueError('incomplete HTTP body')
            return raw if binary else json.loads(raw) if raw else None
        except Exception:
            if method != 'GET' or attempt == 2:
                raise
            time.sleep(1)


def preflight():
    def declarations(path):
        text = path.read_text()
        constants = dict(re.findall(r'^#define\s+(\w+)\s+(\d+)[uUlL]*\s*$', text, re.M))
        body = text.split('typedef struct _LX_QUERY_RESULT', 1)[1].split('{', 1)[1].split('} LX_QUERY_RESULT', 1)[0]
        body = re.sub(r'/\*.*?\*/|//[^\n]*', '', body, flags=re.S)
        for key, value in constants.items():
            body = re.sub(r'\b' + re.escape(key) + r'\b', value, body)
        return [re.sub(r'\s+', '', part) for part in body.split(';') if part.strip()]
    a = declarations(ROOT / 'driver/lxrun.c')
    b = declarations(ROOT / 'driver/lxprobe.c')
    if a != b:
        for index, pair in enumerate(zip(a, b)):
            if pair[0] != pair[1]:
                print(index, pair)
        raise ValueError('query ABI declarations differ')
    print('QUERY_ABI', len(a), 'declarations match')
    driver = (ROOT / 'driver/lxrun.c').read_text()
    assert driver.count('case LX_SYS_WRITEV:') == 1
    assert 'return LxWritevCapture(Context);' in driver
    assert 'candIov' not in driver
    assert 'KAPC_STATE apcState;' not in driver
    assert 'frame + 0x148' in driver and 'frame + 0x150' in driver
    assert driver.count('return LxSysMmapDecoded(Context);') == 1
    assert driver.count('return LxSysMunmapDecoded(Context);') == 1
    assert 'LX_SYS_PREAD64' in driver
    assert '#include "lxmap_runtime.h"' in driver
    print('WRITEV_DISPATCH and frame decoder verified')


def publish():
    preflight()
    paths = ['driver/lxrun.c', 'driver/lxprobe.c', 'driver/lxio_runtime.h', 'driver/lxmap_runtime.h', '.github/workflows/build-driver.yml', 'tools/ci_abi.py']
    head = request('git/ref/heads/main')['object']['sha']
    commit = request('git/commits/' + head)
    tree = []
    for path in paths:
        raw = (ROOT / path).read_bytes()
        blob = request('git/blobs', 'POST', {'content': base64.b64encode(raw).decode(), 'encoding': 'base64'})
        tree.append({'path': path, 'mode': '100644', 'type': 'blob', 'sha': blob['sha']})
    next_tree = request('git/trees', 'POST', {'base_tree': commit['tree']['sha'], 'tree': tree})
    next_commit = request('git/commits', 'POST', {'message': '修正 writev 寄存器解码与异常记录诊断，保留构建产物', 'tree': next_tree['sha'], 'parents': [head]})
    request('git/refs/heads/main', 'PATCH', {'sha': next_commit['sha'], 'force': False})
    OUT.mkdir(parents=True, exist_ok=True)
    state = {'parent': head, 'sha': next_commit['sha'], 'paths': paths}
    (OUT / 'published.json').write_text(json.dumps(state, indent=2))
    print('PUBLISHED', next_commit['sha'], flush=True)
    request('actions/workflows/build-driver.yml/dispatches', 'POST', {'ref': 'main'})
    print('DISPATCHED', flush=True)


def status(run=None):
    if run is None:
        sha = json.loads((OUT / 'published.json').read_text())['sha']
        rows = request('actions/workflows/build-driver.yml/runs?per_page=8')['workflow_runs']
        rows = [row for row in rows if row['head_sha'] == sha]
        if not rows:
            print('AWAITING_RUN', sha)
            return
        run = rows[0]['id']
    data = request('actions/runs/' + str(run))
    print('RUN', data['id'], data['head_sha'], data['status'], data['conclusion'])
    jobs = request('actions/runs/' + str(run) + '/jobs')['jobs']
    for job in jobs:
        active = [s['name'] + ':' + s['status'] for s in job.get('steps', []) if s['status'] != 'completed']
        print('JOB', job['id'], job['status'], job['conclusion'], '; '.join(active[:3]))
    (OUT / ('run-' + str(run) + '.json')).write_text(json.dumps(data, indent=2))


def artifacts(run):
    target = OUT / str(run)
    target.mkdir(parents=True, exist_ok=True)
    records = request('actions/runs/' + str(run) + '/artifacts')
    (target / 'artifacts.json').write_text(json.dumps(records, indent=2))
    for artifact in records['artifacts']:
        path = target / (artifact['name'] + '.zip')
        if not path.exists():
            raw = request('actions/artifacts/' + str(artifact['id']) + '/zip', binary=True)
            archive = zipfile.ZipFile(io.BytesIO(raw))
            if archive.testzip() is not None:
                raise ValueError('artifact CRC error')
            for member in archive.infolist():
                dest = (target / artifact['name'] / member.filename).resolve()
                if not dest.is_relative_to(target.resolve()):
                    raise ValueError('unsafe archive path')
            path.write_bytes(raw)
            archive.extractall(target / artifact['name'])
        print('ARTIFACT', artifact['name'], path.stat().st_size, hashlib.sha256(path.read_bytes()).hexdigest())
    for path in target.rglob('probe.out'):
        text = path.read_text(errors='replace')
        for line in text.splitlines():
            if any(key in line for key in ('POLL1:', 'WRITEV_ABI:', 'WRITE OUTPUT:', 'EXCEPTION_RECORD:', 'writeCount=', 'syscalls (')):
                print(line)


def logs(run):
    for job in request('actions/runs/' + str(run) + '/jobs')['jobs']:
        if job['status'] != 'completed':
            print('LOG_NOT_READY', job['id'])
            continue
        raw = request('actions/jobs/' + str(job['id']) + '/logs', binary=True)
        if b'BlobNotFound' in raw:
            print('LOG_NOT_READY', job['id'])
            continue
        path = OUT / (str(run) + '-' + str(job['id']) + '.log')
        path.write_bytes(raw)
        print('LOG', path, len(raw))
        for line in raw.decode(errors='replace').splitlines():
            if re.search(r'error C\d|error LNK|PROBE EXIT|PROBE TIMEOUT', line):
                print(line)


parser = argparse.ArgumentParser()
parser.add_argument('action', choices=['preflight', 'publish', 'status', 'artifacts', 'logs'])
parser.add_argument('run', nargs='?', type=int)
args = parser.parse_args()
if args.action in ('preflight', 'publish'):
    globals()[args.action]()
else:
    globals()[args.action](args.run)
