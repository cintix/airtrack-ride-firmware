(function () {
  const byId = (id) => document.getElementById(id);

  function formatUptime(ms) {
    const total = Math.floor(ms / 1000);
    const h = Math.floor(total / 3600);
    const m = Math.floor((total % 3600) / 60);
    const s = total % 60;
    return `${h}h ${m}m ${s}s`;
  }

  function formatDuration(seconds) {
    const total = Number(seconds || 0);
    const h = Math.floor(total / 3600);
    const m = Math.floor((total % 3600) / 60);
    const s = total % 60;
    return `${String(h).padStart(2, '0')}:${String(m).padStart(2, '0')}:${String(s).padStart(2, '0')}`;
  }

  function renderToday(today) {
    byId('today-distance').textContent = `${Number(today.distanceKm || 0).toFixed(1)} km`;
    byId('today-duration').textContent = formatDuration(today.durationSeconds);
    byId('today-avg').textContent = `${Number(today.averageSpeedKmh || 0).toFixed(1)} km/t`;
    byId('today-calories').textContent = `${Math.round(Number(today.calories || 0))} kcal`;
  }

  function renderRidesList(rides) {
    const host = byId('rides-list');
    if (!rides || rides.length === 0) {
      host.innerHTML = '<div class="kpi"><span class="label">Ingen ture endnu</span><span class="value">-</span></div>';
      return;
    }

    host.innerHTML = rides
      .map((ride) => {
        return `<div class="kpi"><span class="label">Tur #${ride.id} · ${ride.date}</span><span class="value">${Number(ride.distanceKm).toFixed(1)} km · ${formatDuration(ride.durationSeconds)}</span></div>`;
      })
      .join('');
  }

  function renderRoute(points) {
    if (!Array.isArray(points) || points.length < 2) {
      return;
    }

    const latitudes = points.map((p) => p[0]);
    const longitudes = points.map((p) => p[1]);
    const minLat = Math.min(...latitudes);
    const maxLat = Math.max(...latitudes);
    const minLon = Math.min(...longitudes);
    const maxLon = Math.max(...longitudes);

    const latSpan = Math.max(maxLat - minLat, 0.0001);
    const lonSpan = Math.max(maxLon - minLon, 0.0001);

    const width = 800;
    const height = 240;
    const pad = 24;

    const mapped = points.map((p) => {
      const x = pad + ((p[1] - minLon) / lonSpan) * (width - pad * 2);
      const y = height - (pad + ((p[0] - minLat) / latSpan) * (height - pad * 2));
      return [x, y];
    });

    const path = mapped
      .map((p, index) => `${index === 0 ? 'M' : 'L'}${p[0].toFixed(1)},${p[1].toFixed(1)}`)
      .join(' ');

    const start = mapped[0];
    const end = mapped[mapped.length - 1];

    byId('route-path').setAttribute('d', path);
    byId('route-start').setAttribute('cx', start[0].toFixed(1));
    byId('route-start').setAttribute('cy', start[1].toFixed(1));
    byId('route-end').setAttribute('cx', end[0].toFixed(1));
    byId('route-end').setAttribute('cy', end[1].toFixed(1));
  }

  async function refreshStatus() {
    try {
      const res = await fetch('/api/status');
      const status = await res.json();

      byId('mode').textContent = status.context.toUpperCase();
      byId('sta-ip').textContent = status.sta_ip || '-';
      byId('ap-ip').textContent = status.ap_ip || '-';
      byId('uptime').textContent = formatUptime(status.uptime_ms || 0);
      byId('context-chip').textContent = status.context === 'sta' ? 'STA MODE' : 'AP MODE';
    } catch (err) {
      byId('mode').textContent = 'OFFLINE';
    }
  }

  async function loadRides() {
    try {
      const res = await fetch('/api/rides');
      const data = await res.json();

      renderToday(data.today || {});
      renderRidesList(data.rides || []);

      if (data.rides && data.rides[0]) {
        renderRoute(data.rides[0].points || []);
        byId('route-note').textContent = `Viser rute fra tur #${data.rides[0].id}.`;
      }
    } catch (err) {
      byId('route-note').textContent = 'Kunne ikke hente ture.';
      byId('rides-list').innerHTML = '<div class="kpi"><span class="label">Fejl ved hentning</span><span class="value">-</span></div>';
    }
  }

  function wireUi() {
    byId('open-rides').addEventListener('click', () => {
      const rides = byId('rides-list');
      rides.scrollIntoView({ behavior: 'smooth', block: 'center' });
    });

    byId('export-data').addEventListener('click', () => {
      alert('Eksport/sync hooks klargores i naeste step.');
    });

    byId('edit-profile').addEventListener('click', () => {
      window.location.href = '/setup';
    });

    byId('open-setup').addEventListener('click', () => {
      window.location.href = '/setup';
    });
  }

  wireUi();
  loadRides();
  refreshStatus();
  setInterval(refreshStatus, 5000);
})();
