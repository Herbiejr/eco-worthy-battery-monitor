// solar.js - Only for solar page
function updateSettings() {
    fetch('/status')
    .then(response => {
        if (!response.ok) throw new Error("HTTP " + response.status);
        return response.json();
    })
    .then(data => {
        // SunData section
		document.getElementById("sun-date").innerHTML = formatLongDate(data.sunrise);
	
        document.getElementById("sunrise").innerHTML = formatTime(data.sunrise);
		document.getElementById("sunrise_az").innerHTML = data.sunrise_az + "°";

		document.getElementById("sunset").innerHTML = formatTime(data.sunset);
		document.getElementById("sunset_az").innerHTML = data.sunset_az + "°";

		document.getElementById("daylight").innerHTML = formatDuration(data.daylight);
		// document.getElementById("daylight_diff").innerHTML = formatDaylightDiff(data.daylight_diff);

		document.getElementById("solar_noon_to_12_noon").innerHTML = formatClockTime(data.solar_noon_to_12_noon);
		document.getElementById("solar_noon_altitude").innerHTML = data.solar_noon_altitude + "°";

		document.getElementById("sun_distance").innerHTML = (data.sun_distance / 1000000).toFixed(3) + " Mkm";
		
		document.getElementById("sun-altitude").innerHTML =	data.sun_altitude;
		document.getElementById("sun-azimuth").innerHTML =	data.sun_azimuth;
		document.getElementById("panel-incidence").innerHTML =	data.panel_incidence;

		document.getElementById("totalpower").innerHTML = data.total_watts;
		let totwatt = data.total_watts;
		document.getElementById("totalpower").style.color = totwatt > 0 ? "green" : totwatt < 0 ? "red" : "black";
		
		if(document.activeElement.id !== "panelazimuth"){
			document.getElementById("panelazimuth").value = data.panel_azimuth;
		}

		if(document.activeElement.id !== "paneltilt"){
			document.getElementById("paneltilt").value = data.panel_tilt;
		}
		
    })
    .catch(err => console.error("Settings update error:", err));
}

function setPanelAzimuth() {
    let value = document.getElementById("panelazimuth").value;

    fetch('/setazimuth?value=' + value)
        .then(() => updateStatus());
}

function setPanelTilt() {
    let value = document.getElementById("paneltilt").value;

    fetch('/settilt?value=' + value)
        .then(() => updateStatus());
}

function formatDate(isoString) {
    if(!isoString)
        return "--";

    let d = new Date(isoString);

    return d.toLocaleDateString([], {
        year: 'numeric',
        month: 'short',
        day: 'numeric'
    });
}

function formatLongDate(isoString) {
    if(!isoString)
        return "--";

    let d = new Date(isoString);

    return d.toLocaleDateString([], {
        weekday: 'long',
        year: 'numeric',
        month: 'long',
        day: 'numeric'
    });
}

function formatDateTime(isoString) {
    if(!isoString)
        return "--";

    let d = new Date(isoString);

    return d.toLocaleString([], {
        year: 'numeric',
        month: 'short',
        day: 'numeric',
        hour: '2-digit',
        minute: '2-digit',
        hour12: true
    });
}

function formatTime(isoString) {
    if(!isoString)
        return "--:--";

    let d = new Date(isoString);

    return d.toLocaleTimeString([], {
        hour: '2-digit',
        minute: '2-digit',
        hour12: true
    });
}

function formatDuration(duration) {
    if(!duration)
        return "--";

    return duration;
}

function formatClockTime(timeStr) {
    if(!timeStr)
        return "--";

    const parts = timeStr.split(':');

    if(parts.length !== 3)
        return timeStr;

    let hours = parseInt(parts[0]);
    const minutes = parts[1];
    const seconds = parts[2];

    const ampm = hours >= 12 ? "PM" : "AM";

    hours = hours % 12;
    if(hours === 0)
        hours = 12;

    return `${hours}:${minutes}:${seconds} ${ampm}`;
}

function formatDaylightDiff(diff) {
    if(!diff)
        return "";
    return String(diff).replace("+0:", "+");
}

function setpanel_tilt() {
	let tilt = document.getElementById("panel_tilt").value; 
    var tiltInput = document.getElementById("panel_tilt");
    var tiltValue = parseFloat(tiltInput.value);

    // Enforce min and max limits for manually typed numbers
    if (tiltValue < 0) tiltValue = 0;
    if (tiltValue > 90) tiltValue = 90;

    alert("Panel tilt set to: " + tiltValue + "°");
	// Add your logic here to send the value to your server or hardware
}

// Auto refresh every 2 seconds on settings page
setInterval(updateSettings, 2000);
window.onload = updateSettings;