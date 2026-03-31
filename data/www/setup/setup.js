(function () {
  const byId = (id) => document.getElementById(id);

  function formatUptime(ms) {
    const total = Math.floor(ms / 1000);
    const h = Math.floor(total / 3600);
    const m = Math.floor((total % 3600) / 60);
    const s = total % 60;
    return `${h}h ${m}m ${s}s`;
  }

  function profilePayload() {
    return {
      weightKg: byId('weight').value || '75.0',
      ageYears: byId('age').value || '30',
      isMale: byId('sex').value === 'male' ? '1' : '0',
      restingHeartRateBpm: byId('resting-hr').value || '60',
      timezoneOffsetMinutes: byId('timezone').value || '60',
      stoppedSpeedThresholdKmh: byId('stop-threshold').value || '0.0',
      stoppedDelaySeconds: byId('stop-delay').value || '0'
    };
  }

  function setProfile(data) {
    byId('weight').value = data.weightKg ?? 75;
    byId('age').value = data.ageYears ?? 30;
    byId('sex').value = Number(data.isMale) === 1 ? 'male' : 'female';
    byId('resting-hr').value = data.restingHeartRateBpm ?? 60;
    byId('timezone').value = data.timezoneOffsetMinutes ?? 60;
    byId('stop-threshold').value = data.stoppedSpeedThresholdKmh ?? 0.0;
    byId('stop-delay').value = data.stoppedDelaySeconds ?? 0;
  }

  async function refreshStatus() {
    try {
      const res = await fetch('/api/status');
      const status = await res.json();

      byId('mode').textContent = status.context.toUpperCase();
      byId('ap-ip').textContent = status.ap_ip || '-';
      byId('sta-connected').textContent = status.sta_connected ? 'YES' : 'NO';
      byId('uptime').textContent = formatUptime(status.uptime_ms || 0);
      byId('context-chip').textContent = status.context === 'ap' ? 'AP MODE' : 'STA MODE';
    } catch (err) {
      byId('mode').textContent = 'OFFLINE';
    }
  }

  async function loadProfile() {
    try {
      const res = await fetch('/api/profile');
      const data = await res.json();
      setProfile(data);
    } catch (err) {
      byId('profile-note').textContent = 'Kunne ikke hente profil.';
    }
  }

  async function saveWifi() {
    const payload = new URLSearchParams({
      ssid: byId('ssid').value.trim(),
      password: byId('password').value
    });

    const res = await fetch('/api/setup/wifi', {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: payload
    });

    const data = await res.json();
    if (!res.ok || !data.ok) {
      throw new Error(data.error || 'wifi_save_failed');
    }

    byId('wifi-note').textContent = data.sta_connected
      ? 'Wi-Fi gemt. STA forbindelse etableret.'
      : 'Wi-Fi gemt. STA forbindes stadig/fejlede timeout.';
  }

  async function scanWifi() {
    byId('wifi-note').textContent = 'Scanner netvaerk...';
    const res = await fetch('/api/wifi/scan');
    const data = await res.json();

    if (!res.ok || !data.ok) {
      throw new Error(data.error || 'wifi_scan_failed');
    }

    if (!Array.isArray(data.networks) || data.networks.length === 0) {
      byId('wifi-note').textContent = 'Ingen netvaerk fundet.';
      return;
    }

    const best = data.networks
      .slice(0, 6)
      .map((n) => `${n.ssid || '<hidden>'} (${n.rssi} dBm${n.open ? ', open' : ''})`)
      .join(' | ');

    byId('wifi-note').textContent = `Fundet: ${best}`;
  }

  async function saveProfile() {
    const payload = new URLSearchParams(profilePayload());

    const res = await fetch('/api/profile', {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: payload
    });

    const data = await res.json();
    if (!res.ok || !data.ok) {
      throw new Error(data.error || 'profile_save_failed');
    }

    byId('profile-note').textContent = 'Profil gemt i LittleFS.';
  }

  async function rebootDevice() {
    const res = await fetch('/api/reboot', { method: 'POST' });
    const data = await res.json();

    if (!res.ok || !data.ok) {
      throw new Error(data.error || 'reboot_failed');
    }

    byId('profile-note').textContent = 'Enheden genstarter...';
  }

  function wireUi() {
    byId('save-wifi').addEventListener('click', async () => {
      try {
        await saveWifi();
      } catch (err) {
        byId('wifi-note').textContent = `Fejl: ${err.message}`;
      }
    });

    byId('scan-wifi').addEventListener('click', async () => {
      try {
        await scanWifi();
      } catch (err) {
        byId('wifi-note').textContent = `Fejl: ${err.message}`;
      }
    });

    byId('save-profile').addEventListener('click', async () => {
      try {
        await saveProfile();
      } catch (err) {
        byId('profile-note').textContent = `Fejl: ${err.message}`;
      }
    });

    byId('reboot').addEventListener('click', async () => {
      try {
        await rebootDevice();
      } catch (err) {
        byId('profile-note').textContent = `Fejl: ${err.message}`;
      }
    });
  }

  wireUi();
  loadProfile();
  refreshStatus();
  setInterval(refreshStatus, 5000);
})();
