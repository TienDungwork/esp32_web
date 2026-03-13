    /* ══════════════════════════════════════════════════ */
    /*                TAB SWITCHING                      */
    /* ══════════════════════════════════════════════════ */
    function showTab(id) {
      document.querySelectorAll('.tab-btn').forEach(btn => {
        btn.classList.toggle('active', btn.dataset.tab === id);
      });
      document.querySelectorAll('.tab-pane').forEach(pane => {
        pane.classList.toggle('active', pane.id === id);
      });
    }

    /* ══════════════════════════════════════════════════ */
    /*             COLLAPSIBLE TOGGLE                    */
    /* ══════════════════════════════════════════════════ */
    function toggleCollapse(header) {
      const body = header.nextElementSibling;
      const isOpen = header.classList.contains('open');
      if (isOpen) {
        header.classList.remove('open');
        body.classList.remove('open');
      } else {
        header.classList.add('open');
        body.classList.add('open');
      }
    }

    /* ══════════════════════════════════════════════════ */
    /*             DEVICE STATUS                         */
    /* ══════════════════════════════════════════════════ */
    function setStatusOnline(online) {
      const el = document.getElementById('deviceStatus');
      el.classList.toggle('online', online);
      el.classList.toggle('offline', !online);
    }

    async function refreshNetworkStatus() {
      try {
        const res = await fetch('/api/network/status');
        if (!res.ok) throw new Error('network status error');
        const data = await res.json();
        document.getElementById('currentMode').textContent = data.current_mode || '-';
        document.getElementById('currentIP').textContent = data.ip || '-';
        document.getElementById('wifiStatus').textContent =
          data.wifi_connected ? 'Kết nối' : 'Chưa kết nối';
        document.getElementById('ethStatus').textContent =
          data.ethernet_connected ? 'Kết nối' : 'Chưa kết nối';
        document.getElementById('apSSID').textContent = data.ap_ssid || '-';
        const online = data.wifi_connected || data.ethernet_connected;
        setStatusOnline(online);
        document.getElementById('deviceStatusText').textContent =
          online ? 'Thiết bị đã kết nối mạng' : 'Thiết bị chưa kết nối mạng';
      } catch (e) {
        setStatusOnline(false);
        document.getElementById('deviceStatusText').textContent = 'Không đọc được trạng thái mạng';
      }
    }

    /* ══════════════════════════════════════════════════ */
    /*                 WIFI SCAN                         */
    /* ══════════════════════════════════════════════════ */
    async function scanWiFi() {
      const btn = document.getElementById('scanBtn');
      const list = document.getElementById('wifiList');
      btn.disabled = true;
      btn.innerHTML = '<i class="fas fa-spinner fa-spin"></i> Đang quét...';
      list.innerHTML = '<p style="font-size:13px;color:#6c757d">Đang quét mạng WiFi...</p>';
      try {
        const res = await fetch('/api/wifi/scan');
        const data = await res.json();
        if (!res.ok) throw new Error(data.error || 'scan error');
        const networks = data.networks || [];
        if (!networks.length) {
          list.innerHTML = '<p style="font-size:13px;color:#6c757d">Không tìm thấy mạng nào.</p>';
          return;
        }
        list.innerHTML = '';
        networks.forEach(n => {
          const item = document.createElement('div');
          item.className = 'wifi-item';
          item.innerHTML = `
            <div class="wifi-main">
              <span class="wifi-ssid">${n.ssid}</span>
              <span class="wifi-rssi">${n.rssi} dBm</span>
            </div>
            <button class="btn btn-small btn-primary" type="button"
              onclick="selectWifi('${n.ssid.replace(/'/g, "\\'")}')">Chọn</button>`;
          list.appendChild(item);
        });
      } catch (e) {
        list.innerHTML = `<p style="font-size:13px;color:#dc3545">${e?.message || 'Lỗi khi quét WiFi.'}</p>`;
      } finally {
        btn.disabled = false;
        btn.innerHTML = '<i class="fas fa-search"></i> Quét mạng WiFi';
      }
    }

    function selectWifi(ssid) { document.getElementById('wifiSSID').value = ssid; }

    function toggleStaticIPFields() {
      document.getElementById('staticIPBlock').style.display =
        document.getElementById('useStaticIP').checked ? 'block' : 'none';
    }

    async function connectWifi() {
      const ssid = document.getElementById('wifiSSID').value.trim();
      const password = document.getElementById('wifiPassword').value;
      const useStatic = document.getElementById('useStaticIP').checked;
      const msgEl = document.getElementById('wifiMessage');
      if (!ssid) { msgEl.textContent = 'Vui lòng nhập SSID.'; return; }
      const body = {
        ssid, password, use_static_ip: useStatic,
        static_ip: document.getElementById('staticIP').value.trim(),
        gateway: document.getElementById('staticGateway').value.trim(),
        subnet: document.getElementById('staticSubnet').value.trim(),
        dns1: document.getElementById('staticDNS1').value.trim(),
        dns2: document.getElementById('staticDNS2').value.trim()
      };
      msgEl.textContent = 'Đang gửi cấu hình WiFi...';
      try {
        const res = await fetch('/api/wifi/connect', {
          method: 'POST', headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify(body)
        });
        const data = await res.json();
        msgEl.textContent = data.message || 'Đã gửi cấu hình, đang thử kết nối...';
      } catch (e) { msgEl.textContent = 'Lỗi khi gửi cấu hình WiFi.'; }
    }

    async function loadWifiConfig() {
      const msgEl = document.getElementById('wifiMessage');
      try {
        const res = await fetch('/api/wifi/config');
        if (!res.ok) throw new Error('wifi config fetch failed');
        const data = await res.json();

        document.getElementById('wifiSSID').value = data.ssid || '';
        document.getElementById('wifiPassword').value = data.password || '';

        const useStatic = !!data.use_static_ip;
        document.getElementById('useStaticIP').checked = useStatic;
        toggleStaticIPFields();

        document.getElementById('staticIP').value = data.static_ip || '';
        document.getElementById('staticGateway').value = data.gateway || '';
        document.getElementById('staticSubnet').value = data.subnet || '';
        document.getElementById('staticDNS1').value = data.dns1 || '';
        document.getElementById('staticDNS2').value = data.dns2 || '';

        msgEl.textContent = 'Đã nạp cấu hình WiFi đã lưu.';
      } catch (_) {
        msgEl.textContent = 'Không tải được cấu hình WiFi đã lưu.';
      }
    }

    /* ══════════════════════════════════════════════════ */
    /*                 LAN CONFIG                        */
    /* ══════════════════════════════════════════════════ */
    async function loadLanConfig() {
      const msgEl = document.getElementById('lanMessage');
      msgEl.textContent = 'Đang tải cấu hình LAN...';
      try {
        const res = await fetch('/api/lan');
        if (!res.ok) throw new Error('lan error');
        const data = await res.json();
        document.getElementById('lanIP').value = data.ipAddress || '';
        document.getElementById('lanGW').value = data.gateway || '';
        document.getElementById('lanSubnet').value = data.subnet || '';
        msgEl.textContent = 'Đã tải cấu hình LAN hiện tại.';
      } catch (e) { msgEl.textContent = 'Không đọc được cấu hình LAN.'; }
    }

    async function saveLanConfig() {
      const ip = document.getElementById('lanIP').value.trim();
      const gw = document.getElementById('lanGW').value.trim();
      const sn = document.getElementById('lanSubnet').value.trim();
      const msgEl = document.getElementById('lanMessage');
      if (!ip || !gw || !sn) { msgEl.textContent = 'Vui lòng nhập đầy đủ IP / Gateway / Subnet.'; return; }
      msgEl.textContent = 'Đang lưu cấu hình LAN...';
      try {
        const res = await fetch('/api/lan', {
          method: 'POST', headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ ipAddress: ip, gateway: gw, subnet: sn })
        });
        const data = await res.json();
        msgEl.textContent = data.message || 'Đã lưu, thiết bị có thể restart.';
      } catch (e) { msgEl.textContent = 'Lỗi khi lưu cấu hình LAN.'; }
    }

    /* ══════════════════════════════════════════════════ */
    /*             APP SERVER TCP CONFIG                 */
    /* ══════════════════════════════════════════════════ */
    const APP_DEVICE_TYPES = [
      { name: 'Barrier vào', code: 3 },
      { name: 'Đèn giao thông vào', code: 5 },
      { name: 'Lưới hồng ngoại vào', code: 6 },
      { name: 'Máy in vào', code: 7 },
      { name: 'Camera vào', code: 10 },
      { name: 'Camera nhận diện biển số vào', code: 11 },
      { name: 'Barrier ra', code: 53 },
      { name: 'Đèn giao thông ra', code: 55 },
      { name: 'Lưới hồng ngoại ra', code: 56 },
      { name: 'Máy in ra', code: 57 },
      { name: 'Loa', code: 102 }
    ];

    const APP_DEFAULT_DEVICE_CODE = 3;

    const APP_DEVICE_TYPE_CODE_SET = new Set(APP_DEVICE_TYPES.map(x => x.code));
    let appServerSelectedDeviceCode = APP_DEFAULT_DEVICE_CODE;

    function renderDeviceTypeTable() {
      const tableBody = document.querySelector('#deviceTypeTable tbody');
      if (!tableBody) return;

      tableBody.innerHTML = APP_DEVICE_TYPES.map(item => `
        <tr data-device-code="${item.code}">
          <td>${item.name}</td>
          <td>${item.code}</td>
          <td>
            <span class="device-conn-dot disconnected" id="deviceConnDot-${item.code}"></span>
            <span class="device-conn-label" id="deviceConnLabel-${item.code}">Chưa kết nối</span>
          </td>
          <td>
            <button class="btn btn-small btn-primary" type="button" onclick="selectAppDeviceType(${item.code})">Chọn</button>
          </td>
        </tr>
      `).join('');

      selectAppDeviceType(appServerSelectedDeviceCode, false);
      updateDeviceTypeConnectionIndicators(false);
    }

    function selectAppDeviceType(deviceCode, syncCode = true) {
      if (!APP_DEVICE_TYPE_CODE_SET.has(deviceCode)) return;

      appServerSelectedDeviceCode = deviceCode;
      const idTypeInput = document.getElementById('appServerIdType');
      if (idTypeInput) idTypeInput.value = String(deviceCode);

      document.querySelectorAll('#deviceTypeTable tbody tr').forEach(row => {
        const rowCode = parseInt(row.getAttribute('data-device-code') || '0', 10);
        row.classList.toggle('selected', rowCode === deviceCode);
      });
    }

    function updateDeviceTypeConnectionIndicators(connected, selectedCode = appServerSelectedDeviceCode) {
      APP_DEVICE_TYPES.forEach(item => {
        const isSelected = item.code === selectedCode;
        const isConnected = !!connected && isSelected;

        const dotEl = document.getElementById(`deviceConnDot-${item.code}`);
        const labelEl = document.getElementById(`deviceConnLabel-${item.code}`);
        if (!dotEl || !labelEl) return;

        dotEl.classList.remove('connected', 'disconnected');
        dotEl.classList.add(isConnected ? 'connected' : 'disconnected');
        labelEl.textContent = isConnected ? 'Đã kết nối' : 'Chưa kết nối';
      });
    }

    function getAppServerPayloadFromForm() {
      const ip = document.getElementById('appServerIp')?.value?.trim() || '';
      const port = parseInt(document.getElementById('appServerPort')?.value || '0', 10);
      const idType = parseInt(document.getElementById('appServerIdType')?.value || String(APP_DEFAULT_DEVICE_CODE), 10);
      const autoReconnect = !!document.getElementById('appServerAutoReconnect')?.checked;
      return {
        ip,
        port: Number.isNaN(port) ? 0 : port,
        id_type: Number.isNaN(idType) ? APP_DEFAULT_DEVICE_CODE : idType,
        // Mã thiết bị được chọn trong bảng (DeviceType)
        selected_device_code: appServerSelectedDeviceCode,
        auto_reconnect: autoReconnect
      };
    }

    function applyAppServerConfigToForm(data) {
      if (!data || typeof data !== 'object') return;
      document.getElementById('appServerIp').value = data.ip || '';
      document.getElementById('appServerPort').value = data.port || '';
      const selectedCode = parseInt(data.selected_device_code || data.id_type || APP_DEFAULT_DEVICE_CODE, 10);
      if (APP_DEVICE_TYPE_CODE_SET.has(selectedCode)) {
        selectAppDeviceType(selectedCode, false);
      } else {
        selectAppDeviceType(APP_DEFAULT_DEVICE_CODE, false);
      }
      document.getElementById('appServerIdType').value = data.id_type || appServerSelectedDeviceCode;
      document.getElementById('appServerAutoReconnect').checked = !!data.auto_reconnect;
    }

    function setAppServerStatusText(connected, confirmed, lastError = '') {
      const stateEl = document.getElementById('appServerConnState');
      const bannerEl = document.getElementById('serverStatusBanner');
      const inlineStateEl = document.getElementById('appServerConnStateInline');
      const inlineDotEl = document.getElementById('appServerConnDotInline');
      if (!stateEl) return;

      if (bannerEl) {
        bannerEl.classList.remove('connected', 'disconnected', 'tcp-only');
        if (connected) {
          bannerEl.classList.add('connected');
        } else {
          bannerEl.classList.add('disconnected');
        }
      }

      if (connected) {
        stateEl.textContent = confirmed ? 'Đã kết nối server và đã xác nhận' : 'Đã kết nối server';
        stateEl.style.color = '';
      } else {
        stateEl.textContent = lastError ? `Chưa kết nối: ${lastError}` : 'Chưa kết nối';
        stateEl.style.color = '';
      }

      if (inlineStateEl) {
        inlineStateEl.textContent = connected ? 'Đã kết nối server' : (lastError || 'Chưa kết nối');
        inlineStateEl.className = 'app-server-inline-status ' + (connected ? 'connected' : 'disconnected');
      }
      if (inlineDotEl) {
        inlineDotEl.className = 'app-server-inline-dot ' + (connected ? 'connected' : 'disconnected');
      }

      updateDeviceTypeConnectionIndicators(connected && confirmed, appServerSelectedDeviceCode);
    }

    async function loadAppServerConfig() {
      const msgEl = document.getElementById('appServerMessage');
      msgEl.textContent = 'Đang tải cấu hình server...';
      try {
        const res = await fetch('/api/app-server/config');
        if (!res.ok) throw new Error('config error');
        const data = await res.json();
        applyAppServerConfigToForm(data);
        msgEl.textContent = 'Đã tải cấu hình server.';
      } catch (_) {
        msgEl.textContent = 'Không tải được cấu hình server.';
      }

      await refreshAppServerStatus();
    }

    async function refreshAppServerStatus() {
      try {
        const res = await fetch('/api/app-server/status');
        if (!res.ok) throw new Error('status error');
        const data = await res.json();
        const selectedCode = parseInt(data.selected_device_code || data.id_type || appServerSelectedDeviceCode, 10);
        if (APP_DEVICE_TYPE_CODE_SET.has(selectedCode)) {
          selectAppDeviceType(selectedCode, false);
        }
        const connected = !!data.connected;
        const confirmed = !!data.connection_confirmed;
        setAppServerStatusText(connected, confirmed, data.last_error || '');
      } catch (_) {
        setAppServerStatusText(false, false, 'không đọc được trạng thái');
      }
    }

    async function saveAppServerConfig() {
      const msgEl = document.getElementById('appServerMessage');
      const payload = {
        ...getAppServerPayloadFromForm(),
        enabled: false
      };

      if (!payload.ip) { msgEl.textContent = 'Vui lòng nhập IP server.'; return; }
      if (payload.port < 1 || payload.port > 65535) { msgEl.textContent = 'Port phải từ 1..65535.'; return; }
      if (!APP_DEVICE_TYPE_CODE_SET.has(payload.selected_device_code)) { msgEl.textContent = 'Thiết bị chưa hợp lệ.'; return; }

      msgEl.textContent = 'Đang lưu cấu hình server...';
      try {
        const res = await fetch('/api/app-server/config', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify(payload)
        });
        const data = await res.json();
        msgEl.textContent = res.ok ? (data.message || 'Đã lưu cấu hình server.') : (data.error || 'Lưu thất bại.');
      } catch (_) {
        msgEl.textContent = 'Lỗi khi lưu cấu hình server.';
      }
    }

    async function connectAppServer() {
      const msgEl = document.getElementById('appServerMessage');
      const payload = getAppServerPayloadFromForm();

      if (!payload.ip) { msgEl.textContent = 'Vui lòng nhập IP server.'; return; }
      if (payload.port < 1 || payload.port > 65535) { msgEl.textContent = 'Port phải từ 1..65535.'; return; }

      msgEl.textContent = 'Đang kết nối tới server...';
      try {
        const res = await fetch('/api/app-server/connect', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          // Kết nối TCP: chỉ cần IP + Port. Gói Code=1 sẽ gửi khi bấm "Gửi yêu cầu kết nối".
          body: JSON.stringify({
            ip: payload.ip,
            port: payload.port
          })
        });
        const data = await res.json();
        msgEl.textContent = res.ok
          ? (data.message || 'Đã kết nối server.')
          : (data.error || data.message || 'Kết nối thất bại.');
      } catch (_) {
        msgEl.textContent = 'Không kết nối được server.';
      }

      await refreshAppServerStatus();
    }

    async function sendConnectRequestPacket() {
      const msgEl = document.getElementById('appServerMessage');
      msgEl.textContent = 'Đang gửi gói yêu cầu kết nối (Code=1)...';
      try {
        const res = await fetch('/api/app-server/send-connect-request', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({
            selected_device_code: appServerSelectedDeviceCode
          })
        });
        const data = await res.json();
        msgEl.textContent = res.ok
          ? (data.message || 'Đã gửi gói yêu cầu kết nối.')
          : (data.error || 'Gửi gói yêu cầu kết nối thất bại.');
      } catch (_) {
        msgEl.textContent = 'Không gửi được gói yêu cầu kết nối.';
      }

      await refreshAppServerStatus();
    }

    async function disconnectAppServer() {
      const msgEl = document.getElementById('appServerMessage');
      msgEl.textContent = 'Đang ngắt kết nối server...';
      try {
        const res = await fetch('/api/app-server/disconnect', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' }
        });
        const data = await res.json();
        msgEl.textContent = res.ok ? (data.message || 'Đã ngắt kết nối.') : (data.error || 'Ngắt kết nối thất bại.');
      } catch (_) {
        msgEl.textContent = 'Không gửi được lệnh ngắt kết nối.';
      }

      await refreshAppServerStatus();
    }

    /* ══════════════════════════════════════════════════ */
    /*          FEATURES – SIDEBAR SELECTION             */
    /* ══════════════════════════════════════════════════ */
    function selectFeature(featureId, el) {
      document.querySelectorAll('.sidebar-item').forEach(s => s.classList.remove('active'));
      el.classList.add('active');
      document.querySelectorAll('.feature-panel').forEach(p => p.classList.remove('active'));
      const panel = document.getElementById('panel-' + featureId);
      if (panel) panel.classList.add('active');

      if (featureId === 'barrier' || featureId === 'traffic') {
        switchExclusiveDeviceControlMode(featureId);
      }
    }

    async function switchExclusiveDeviceControlMode(featureId) {
      const mode = featureId === 'barrier' ? 'barrier' : (featureId === 'traffic' ? 'traffic' : 'none');
      if (mode === 'none') return;

      try {
        const res = await fetch('/api/device/control-mode', {
          method: 'POST', headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ mode })
        });
        const data = await res.json();
        if (!res.ok) throw new Error(data.error || 'switch mode failed');

        if (mode === 'barrier') {
          const trafficMsgEl = document.getElementById('trafficMessage');
          if (trafficMsgEl) trafficMsgEl.textContent = 'Đã ngắt đèn giao thông để chuyển sang chế độ Barrier.';
        }
        if (mode === 'traffic') {
          const barrierMsgEl = document.getElementById('barrierMessage');
          const barrierStatusEl = document.getElementById('barrierStatus');
          if (barrierMsgEl) barrierMsgEl.textContent = 'Đã ngắt Barrier để chuyển sang chế độ đèn giao thông.';
          if (barrierStatusEl) barrierStatusEl.textContent = 'Đã ngắt do chuyển mode';
        }

        refreshSensors(false);
      } catch (_) {
        // Keep UX silent here because user may still control directly via action buttons.
      }
    }

    /* ══════════════════════════════════════════════════ */
    /*          FEATURE: LED CONFIG                      */
    /* ══════════════════════════════════════════════════ */
    async function sendLedConfig() {
      const boardCount = document.getElementById('ledBoardCount').value;
      const lineSpacing = parseInt(document.getElementById('ledLineSpacing')?.value ?? '-1', 10);
      const lines = [];
      for (let i = 1; i <= 5; i++) {
        lines.push({
          line: i,
          fixed: document.getElementById('ledFixed' + i)?.value || '',
          text: document.getElementById('ledText' + i).value,
          fontSize: parseFloat(document.getElementById('ledSize' + i).value) || 1.6,
          color: document.getElementById('ledColor' + i)?.value || '#00ff00'
        });
      }
      const payload = {
        boardCount: parseInt(boardCount, 10) || 1,
        lineSpacing: Number.isNaN(lineSpacing) ? -1 : lineSpacing,
        lines
      };
      const msgEl = document.getElementById('ledMessage');
      msgEl.textContent = 'Đang gửi cấu hình LED...';
      try {
        const res = await fetch('/api/led/config', {
          method: 'POST', headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify(payload)
        });
        const raw = await res.text();
        let data = {};
        try { data = raw ? JSON.parse(raw) : {}; } catch (_) { data = { error: raw || 'Invalid response' }; }
        if (!res.ok) { msgEl.textContent = `Lỗi ${res.status}: ${data.error || 'Gửi cấu hình LED thất bại.'}`; return; }
        msgEl.textContent = data.message || 'Đã gửi cấu hình LED.';
      } catch (e) { msgEl.textContent = e?.message ? `Lỗi: ${e.message}` : 'Lỗi khi gửi cấu hình LED.'; }
    }

    function applyLedConfigToForm(data) {
      if (!data || typeof data !== 'object') return;

      const boardCount = parseInt(data.boardCount, 10);
      if (!Number.isNaN(boardCount)) {
        document.getElementById('ledBoardCount').value = boardCount;
      }

      const lineSpacing = parseInt(data.lineSpacing, 10);
      document.getElementById('ledLineSpacing').value = Number.isNaN(lineSpacing) ? -1 : lineSpacing;

      const lines = Array.isArray(data.lines) ? data.lines : [];
      for (let i = 1; i <= 5; i++) {
        const item = lines.find(x => Number(x?.line) === i) || {};
        document.getElementById('ledFixed' + i).value = item.fixed || '';
        document.getElementById('ledText' + i).value = item.text || '';
        document.getElementById('ledSize' + i).value = item.fontSize || 1.6;
        document.getElementById('ledColor' + i).value = item.color || '#00ff00';
      }
    }

    async function loadLedConfig() {
      const msgEl = document.getElementById('ledMessage');
      try {
        const res = await fetch('/api/led/config');
        if (!res.ok) throw new Error('Không tải được cấu hình LED');
        const data = await res.json();
        applyLedConfigToForm(data);
      } catch (e) {
        msgEl.textContent = 'Không tải được cấu hình LED đã lưu.';
      }
    }

    /* ══════════════════════════════════════════════════ */
    /*          FEATURE: BARRIER CONTROL                 */
    /* ══════════════════════════════════════════════════ */
    async function controlBarrier(action) {
      const msgEl = document.getElementById('barrierMessage');
      const statusEl = document.getElementById('barrierStatus');
      const labels = { open: 'Đang gửi xung MỞ...', close: 'Đang gửi xung ĐÓNG...', stop: 'Đang gửi xung PAUSE...' };
      statusEl.textContent = labels[action] || action;
      msgEl.textContent = '';
      try {
        const res = await fetch('/api/barrier/control', {
          method: 'POST', headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ action })
        });
        const data = await res.json();
        if (res.ok) {
          const doneLabels = { open: 'Đã gửi MỞ ✅', close: 'Đã gửi ĐÓNG ✅', stop: 'Đã gửi PAUSE ✅' };
          statusEl.textContent = doneLabels[action] || 'OK';
          msgEl.textContent = data.message || 'Thành công!';
        } else {
          statusEl.textContent = 'Lỗi';
          msgEl.textContent = data.error || 'Gửi lệnh thất bại.';
        }
      } catch (e) { statusEl.textContent = 'Lỗi kết nối'; msgEl.textContent = 'Không gửi được lệnh.'; }
    }

    /* ══════════════════════════════════════════════════ */
    /*          FEATURE: TRAFFIC LIGHT                   */
    /* ══════════════════════════════════════════════════ */
    let trafficSelectionDirty = false;

    async function controlTrafficLight() {
      const state = document.querySelector('input[name="trafficState"]:checked')?.value || 'red';
      const msgEl = document.getElementById('trafficMessage');
      msgEl.textContent = 'Đang gửi lệnh...';
      try {
        const res = await fetch('/api/traffic-light/control', {
          method: 'POST', headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ state })
        });
        const data = await res.json();
        msgEl.textContent = res.ok ? (data.message || 'Đã áp dụng!') : (data.error || 'Lỗi.');
        if (res.ok) {
          trafficSelectionDirty = false;
        }
      } catch (e) { msgEl.textContent = 'Không gửi được lệnh.'; }
    }

    function syncBarrierAndTrafficState(data) {
      if (!data || typeof data !== 'object') return;

      const barrierLabelMap = {
        open: 'Barie đang mở',
        close: 'Barie đang đóng',
        stop: 'Barie đang dừng'
      };
      const barrierState = String(data.barrier || '').toLowerCase();
      const barrierStatusEl = document.getElementById('barrierStatus');
      if (barrierStatusEl && barrierLabelMap[barrierState]) {
        barrierStatusEl.textContent = barrierLabelMap[barrierState];
      }

      const trafficState = String(data.traffic_light || '').toLowerCase();
      const trafficPanelActive = !!document.getElementById('panel-traffic')?.classList.contains('active');
      const shouldSyncTrafficRadio = !(trafficPanelActive && trafficSelectionDirty);

      if (shouldSyncTrafficRadio && (trafficState === 'green' || trafficState === 'red' || trafficState === 'yellow')) {
        const trafficRadio = document.querySelector(`input[name="trafficState"][value="${trafficState}"]`);
        if (trafficRadio) trafficRadio.checked = true;
      }

      const trafficMsgEl = document.getElementById('trafficMessage');
      if (trafficMsgEl) {
        if (trafficState === 'red_flash') {
          trafficMsgEl.textContent = 'Đèn đỏ đang nhấp nháy';
        } else if (trafficState === 'off') {
          trafficMsgEl.textContent = 'Đèn giao thông đang tắt';
        }
      }
    }

    /* ══════════════════════════════════════════════════ */
    /*          FEATURE: BEAM / SENSOR                   */
    /* ══════════════════════════════════════════════════ */
    async function refreshSensors(showMessage = true) {
      const msgEl = document.getElementById('beamMessage');
      if (showMessage) msgEl.textContent = 'Đang đọc...';
      try {
        const res = await fetch('/api/device/status');
        const data = await res.json();

        syncBarrierAndTrafficState(data);

        const updateBeamStatus = (elId, rawValue) => {
          const el = document.getElementById(elId);
          if (!el) return;
          if (rawValue === 0 || rawValue === '0') {
            el.textContent = 'Có vật cản';
            el.className = 'sensor-status warn';
            return;
          }
          if (rawValue === 1 || rawValue === '1') {
            el.textContent = 'Không có vật cản';
            el.className = 'sensor-status ok';
            return;
          }
          el.textContent = 'N/A';
          el.className = 'sensor-status';
        };

        // Layout/order requirement:
        // Beam 1 = IO40 (beam_pwm2), Beam 2 = IO4 (beam_a0), Beam 3 = IO39 (beam_pwm1)
        updateBeamStatus('beam1Status', data.beam_pwm2);
        updateBeamStatus('beam2Status', data.beam_a0);
        updateBeamStatus('beam3Status', data.beam_pwm1);

        const updateButtonStatus = (elId, rawValue) => {
          const el = document.getElementById(elId);
          if (!el) return;
          if (rawValue === 0 || rawValue === '0') {
            el.textContent = 'Đang nhấn ⚠️';
            el.className = 'sensor-status warn';
            return;
          }
          if (rawValue === 1 || rawValue === '1') {
            el.textContent = 'Đã nhả ✅';
            el.className = 'sensor-status ok';
            return;
          }
          el.textContent = 'N/A';
          el.className = 'sensor-status';
        };

        updateButtonStatus('btnOpenStatus', data.button_open ?? data.btn_open);
        updateButtonStatus('btnCloseStatus', data.button_close ?? data.btn_close);
        updateButtonStatus('btnPauseStatus', data.button_stop ?? data.btn_stop);
        if (showMessage) msgEl.textContent = 'Đã cập nhật.';
      } catch (e) {
        if (showMessage) msgEl.textContent = 'Không đọc được trạng thái.';
      }
    }

    /* ══════════════════════════════════════════════════ */
    /*          FEATURE: OTA UPDATE                      */
    /* ══════════════════════════════════════════════════ */
    async function loadCurrentFirmwareVersion() {
      const versionEl = document.getElementById('otaCurrentVersion');
      if (!versionEl) return;

      versionEl.textContent = 'Đang đọc...';
      try {
        const res = await fetch('/api/network/status');
        if (!res.ok) throw new Error('network status error');

        const data = await res.json();
        const fwVersion = data.firmware_version || 'dev';
        const fwBuild = data.firmware_build || '';
        versionEl.textContent = fwBuild ? `${fwVersion} (${fwBuild})` : fwVersion;
      } catch (_) {
        versionEl.textContent = 'Không đọc được';
      }
    }

    async function startOTA() {
      const url = document.getElementById('otaUrl').value.trim();
      const msgEl = document.getElementById('otaMessage');
      if (!url) { msgEl.textContent = 'Vui lòng nhập URL firmware.'; return; }
      if (!confirm('Thiết bị sẽ RESTART sau khi cập nhật. Bạn chắc chắn?')) return;
      msgEl.textContent = 'Đang gửi lệnh OTA...';
      try {
        const res = await fetch('/api/ota/update', {
          method: 'POST', headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ url })
        });
        const data = await res.json();
        msgEl.textContent = res.ok ? (data.message || 'Đã gửi lệnh, thiết bị đang cập nhật...') : (data.error || 'Lỗi.');
      } catch (e) { msgEl.textContent = 'Không gửi được lệnh OTA.'; }
    }

    /* ══════════════════════════════════════════════════ */
    /*                    INIT                           */
    /* ══════════════════════════════════════════════════ */
    window.addEventListener('load', () => {
      renderDeviceTypeTable();
      document.querySelectorAll('input[name="trafficState"]').forEach((input) => {
        input.addEventListener('change', () => {
          trafficSelectionDirty = true;
        });
      });

      refreshNetworkStatus();
      loadCurrentFirmwareVersion();
      loadWifiConfig();
      loadAppServerConfig();
      loadLedConfig();
      refreshSensors(false);
      setInterval(() => refreshSensors(false), 300);
      setInterval(() => refreshAppServerStatus(), 3000);
    });
