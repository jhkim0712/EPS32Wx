// ===================== 공통 =====================
function showStatus(message, type = 'success') {
    const statusDiv = document.getElementById('status');
    statusDiv.innerHTML = `<div class="status ${type}">${message}</div>`;
    setTimeout(() => { statusDiv.innerHTML = ''; }, 5000);
}

function postJson(url, body) {
    return fetch(url, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(body)
    }).then(r => r.json());
}

// ===================== 탭 전환 =====================
document.querySelectorAll('.tab-btn').forEach(btn => {
    btn.addEventListener('click', () => {
        document.querySelectorAll('.tab-btn').forEach(b => b.classList.remove('active'));
        document.querySelectorAll('.tab-panel').forEach(p => p.classList.remove('active'));
        btn.classList.add('active');
        document.getElementById('tab-' + btn.dataset.tab).classList.add('active');

        if (btn.dataset.tab === 'system') loadSystemTab();
        if (btn.dataset.tab === 'pictures') loadPicturesTab();
    });
});

// ===================== WiFi 탭 =====================
let wifiNetworks = [];

function scanWiFi() {
    const wifiList = document.getElementById('wifiList');
    wifiList.style.display = 'block';
    wifiList.innerHTML = '<div class="loading">WiFi 네트워크를 스캔하는 중...</div>';

    fetch('/api/scan')
        .then(response => response.json())
        .then(data => {
            if (data.success) {
                wifiNetworks = data.networks;
                displayWiFiNetworks(data.networks);
            } else {
                wifiList.innerHTML = '<div class="loading">스캔 실패</div>';
            }
        })
        .catch(() => { wifiList.innerHTML = '<div class="loading">스캔 중 오류 발생</div>'; });
}
document.getElementById('scanBtn').addEventListener('click', scanWiFi);

function displayWiFiNetworks(networks) {
    const wifiList = document.getElementById('wifiList');
    if (networks.length === 0) {
        wifiList.innerHTML = '<div class="loading">WiFi 네트워크를 찾을 수 없습니다</div>';
        return;
    }
    let html = '';
    networks.forEach(network => {
        const signalStrength = getSignalStrengthText(network.rssi);
        const security = network.authmode > 0 ? '🔒' : '🔓';
        html += `<div class="wifi-item" data-ssid="${network.ssid}">${security} ${network.ssid}<span class="signal-strength">${signalStrength}</span></div>`;
    });
    wifiList.innerHTML = html;
    wifiList.querySelectorAll('.wifi-item').forEach(el => {
        el.addEventListener('click', () => selectWiFi(el.dataset.ssid));
    });
}

function getSignalStrengthText(rssi) {
    if (rssi > -50) return '📶 매우 강함';
    if (rssi > -60) return '📶 강함';
    if (rssi > -70) return '📶 보통';
    return '📶 약함';
}

function selectWiFi(ssid) {
    document.getElementById('ssid').value = ssid;
    document.getElementById('wifiList').style.display = 'none';
    const network = wifiNetworks.find(n => n.ssid === ssid);
    if (network && network.authmode === 0) {
        document.getElementById('password').value = '';
    }
}

document.getElementById('configForm').addEventListener('submit', function (e) {
    e.preventDefault();
    const formData = new FormData(this);
    const config = {
        ssid: formData.get('ssid'),
        password: formData.get('password'),
        apiKey: formData.get('apiKey'),
        cityName: formData.get('cityName')
    };

    const submitBtn = this.querySelector('button[type="submit"]');
    submitBtn.disabled = true;
    submitBtn.textContent = '설정 저장 중...';

    postJson('/api/config', config)
        .then(data => {
            if (data.success) {
                showStatus('설정이 성공적으로 저장되었습니다! 디바이스가 재시작됩니다...', 'success');
                setTimeout(() => { window.location.href = '/'; }, 3000);
            } else {
                showStatus('설정 저장에 실패했습니다: ' + (data.message || '알 수 없는 오류'), 'error');
                submitBtn.disabled = false;
                submitBtn.textContent = '설정 저장';
            }
        })
        .catch(() => {
            showStatus('설정 저장 중 오류가 발생했습니다.', 'error');
            submitBtn.disabled = false;
            submitBtn.textContent = '설정 저장';
        });
});

// ===================== System 탭 =====================
function populateHourSelect(sel, value) {
    sel.innerHTML = '';
    for (let h = 0; h < 24; h++) {
        const opt = document.createElement('option');
        opt.value = h;
        opt.textContent = String(h).padStart(2, '0') + ':00';
        if (h === value) opt.selected = true;
        sel.appendChild(opt);
    }
}

let systemTabLoaded = false;
function loadSystemTab() {
    fetch('/api/status').then(r => r.json()).then(data => {
        document.getElementById('statWifi').textContent = data.wifiConnected
            ? `${data.ssid} (${data.ip})` : (data.apMode ? 'AP 설정 모드' : '연결 안됨');
        document.getElementById('statSd').textContent = data.sdMounted ? '마운트됨' : '없음';
        document.getElementById('statVersion').textContent = data.firmwareVersion || '-';
    }).catch(() => {});

    fetch('/api/system').then(r => r.json()).then(data => {
        document.getElementById('brightness').value = data.brightness;
        document.getElementById('brightnessVal').textContent = data.brightness;
        document.getElementById('timezone').value = data.timezone || '';
        document.getElementById('nightDimEnabled').checked = !!data.nightDimEnabled;
        populateHourSelect(document.getElementById('nightDimStart'), data.nightDimStart);
        populateHourSelect(document.getElementById('nightDimEnd'), data.nightDimEnd);
        document.getElementById('otaUrl').value = data.otaManifestUrl || '';

        document.querySelectorAll('#rotationBtns .chip').forEach(btn => {
            btn.classList.toggle('active', Number(btn.dataset.rot) === Number(data.rotation));
        });

        systemTabLoaded = true;
    }).catch(() => {});
}

document.getElementById('brightness').addEventListener('input', (e) => {
    document.getElementById('brightnessVal').textContent = e.target.value;
});
document.getElementById('brightness').addEventListener('change', (e) => {
    postJson('/api/system', { brightness: Number(e.target.value) }).catch(() => {});
});

document.querySelectorAll('#rotationBtns .chip').forEach(btn => {
    btn.addEventListener('click', () => {
        document.querySelectorAll('#rotationBtns .chip').forEach(b => b.classList.remove('active'));
        btn.classList.add('active');
        postJson('/api/system', { rotation: Number(btn.dataset.rot) }).catch(() => {});
    });
});

document.getElementById('saveSystemBtn').addEventListener('click', () => {
    const body = {
        timezone: document.getElementById('timezone').value,
        nightDimEnabled: document.getElementById('nightDimEnabled').checked,
        nightDimStart: Number(document.getElementById('nightDimStart').value),
        nightDimEnd: Number(document.getElementById('nightDimEnd').value),
    };
    postJson('/api/system', body).then(data => {
        showStatus(data.success ? '시스템 설정이 저장되었습니다.' : '저장 실패', data.success ? 'success' : 'error');
    }).catch(() => showStatus('저장 중 오류가 발생했습니다.', 'error'));
});

// ---- OTA ----
document.getElementById('otaSaveBtn').addEventListener('click', () => {
    postJson('/api/system', { otaManifestUrl: document.getElementById('otaUrl').value }).then(data => {
        showStatus(data.success ? '매니페스트 URL이 저장되었습니다.' : '저장 실패', data.success ? 'success' : 'error');
    }).catch(() => showStatus('저장 중 오류가 발생했습니다.', 'error'));
});

document.getElementById('otaCheckBtn').addEventListener('click', () => {
    const status = document.getElementById('otaStatus');
    status.textContent = '확인하는 중...';
    fetch('/api/ota/check').then(r => r.json()).then(data => {
        if (data.available) {
            status.textContent = `새 버전 ${data.availableVersion} 사용 가능 (현재 ${data.currentVersion})`;
            document.getElementById('otaStartBtn').disabled = false;
        } else if (data.error === 'no_manifest_url') {
            status.textContent = '먼저 매니페스트 URL을 저장하세요.';
        } else if (data.error) {
            status.textContent = '확인 실패';
        } else {
            status.textContent = `최신 버전입니다 (${data.currentVersion})`;
        }
    }).catch(() => { status.textContent = '확인 중 오류 발생'; });
});

let otaPollTimer = null;
document.getElementById('otaStartBtn').addEventListener('click', function () {
    this.disabled = true;
    postJson('/api/ota/start', {}).then(data => {
        if (!data.success) {
            showStatus('업데이트 시작 실패: ' + (data.message || ''), 'error');
            this.disabled = false;
            return;
        }
        if (otaPollTimer) clearInterval(otaPollTimer);
        otaPollTimer = setInterval(pollOtaStatus, 1000);
    });
});

function pollOtaStatus() {
    fetch('/api/ota/status').then(r => r.json()).then(data => {
        document.getElementById('otaProgress').style.width = data.progress + '%';
        // ota_status_t: 0 IDLE, 1 DOWNLOADING, 2 INSTALLING, 3 SUCCESS, 4 FAILED, 5 ROLLBACK
        const labels = ['대기 중', '다운로드 중', '설치 중', '완료, 재시작 중...', '실패', '롤백'];
        document.getElementById('otaStatus').textContent = (labels[data.status] || '') + ` (${data.progress}%)`;
        if (data.status === 3 || data.status === 4) {
            clearInterval(otaPollTimer);
            otaPollTimer = null;
        }
    }).catch(() => {});
}

// ===================== Pictures 탭 =====================
document.getElementById('slideshowInterval').addEventListener('input', (e) => {
    document.getElementById('slideshowIntervalVal').textContent = e.target.value;
});

function loadPicturesTab() {
    fetch('/api/gallery/list').then(r => r.json()).then(data => {
        const listEl = document.getElementById('fileList');
        if (!data.sdMounted) {
            listEl.innerHTML = '<div class="loading">SD 카드가 연결되어 있지 않습니다</div>';
            return;
        }
        if (!data.files || data.files.length === 0) {
            listEl.innerHTML = '<div class="loading">/photos 폴더에 사진이 없습니다</div>';
            return;
        }
        listEl.innerHTML = data.files.map(f =>
            `<div class="file-item"><span>${f.name}</span><span>${(f.size / 1024).toFixed(0)} KB</span></div>`
        ).join('');
    }).catch(() => {
        document.getElementById('fileList').innerHTML = '<div class="loading">불러오기 실패</div>';
    });
}

document.getElementById('savePicturesBtn').addEventListener('click', () => {
    const body = {
        enabled: document.getElementById('slideshowEnabled').checked,
        intervalSec: Number(document.getElementById('slideshowInterval').value)
    };
    postJson('/api/gallery/config', body).then(data => {
        showStatus(data.success ? 'Pictures 설정이 저장되었습니다.' : '저장 실패', data.success ? 'success' : 'error');
    }).catch(() => showStatus('저장 중 오류가 발생했습니다.', 'error'));
});

// ===================== 초기 로드 =====================
window.addEventListener('load', function () {
    fetch('/api/status')
        .then(response => response.json())
        .then(data => {
            if (data.cityName) {
                document.getElementById('cityName').value = data.cityName;
            }
        })
        .catch(() => {});
});
