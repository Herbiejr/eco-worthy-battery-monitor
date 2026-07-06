// settings.js - Only for settings page
let lastIP = "";
function updateSettings() {
    fetch('/status')
    .then(response => {
        if (!response.ok) throw new Error("HTTP " + response.status);
        return response.json();
    })
    .then(data => {
        // Logging section
        let logStatus = document.getElementById("loggingstatus");
        if (logStatus) {
            logStatus.innerHTML = data.logging ? "ON" : "OFF";
            logStatus.style.color = data.logging ? "green" : "red";
        }

        let logSize = document.getElementById("logsize");
        if (logSize) logSize.innerHTML = data.log_size;

        // Network / WiFi section
        let wifiStatus = document.getElementById("wifi-status");
        if (wifiStatus) {
            wifiStatus.innerHTML = data.wifi_connected ? "Connected" : "Disconnected";
            wifiStatus.style.color = data.wifi_connected ? "green" : "red";
        }

        const fields = ["ssid", "ip", "ap_ip", "sta_ip", "public_ip", "gateway", "rssi"];
		if (lastIP === "") {
			// First update after page load
			lastIP = data.sta_ip;
		}
		else if (lastIP !== data.sta_ip) {
			console.log("STA IP changed:", lastIP, "->", data.sta_ip);
			lastIP = data.sta_ip;

			// Give the ESP a moment to finish reconnecting
			let ota_hostname = document.getElementById("hostname");
			setTimeout(() => {window.location.href = "http://" + data.hostname;}, 1000);
		}
		
        fields.forEach(id => {
            let el = document.getElementById(id);
            if (el) {
                if (id === "rssi") {
                    el.innerHTML = data.rssi;
                } else {
                    el.innerHTML = data[id.replace('-', '_')] || "Unknown";
                }
            }
        });
		
		let host = document.getElementById("hostname");
		if(host)
			host.innerHTML = data.hostname;
		
		document.getElementById("current_location").innerHTML =
			data.city + ", " +
			data.province + ", " +
			data.country;

		document.getElementById("geoname_id").innerHTML =
			data.geoname_id;

		document.getElementById("current_lat").innerHTML =
			data.latitude;

		document.getElementById("current_long").innerHTML =
			data.longitude;
	
        // Update log interval input if not focused
        let intervalInput = document.getElementById("loginterval");
        if (intervalInput && document.activeElement.id !== "loginterval") {
            intervalInput.value = data.log_interval;
        }
    })
    .catch(err => console.error("Settings update error:", err));

	//fetch('/detectlocation')
}

function setLogInterval() {
	let seconds = document.getElementById("loginterval").value; 
	fetch('/setinterval?sec=' + seconds).then(
		() => updateSettings()
	);}
	
function startLogging() {
	fetch('/startlogging').then(
		() => updateSettings()
	);}

function stopLogging() {
	fetch('/stoplogging').then(
		() => updateSettings()
	);}

function clearLog() {
	if(confirm("Delete all logged data?"))	{
		fetch('/clearlog').then(
			() => updateSettings()
			);
	}
}
//function ledOn() {fetch('/ledon').then(() => updateSettings());}
//function ledOff() {fetch('/ledoff').then(() => updateSettings());}

function resetWifi(){
	if(confirm("Clear WiFi credentials?")){
		fetch('/resetwifi');
	}
}

async function loadNearbyCities(){
    const response = await fetch("/nearbycities");
    const cities = await response.json();
    const list = document.getElementById("citySelect");

    list.innerHTML = "";

    cities.forEach(city =>{
        const option = document.createElement("option");
        option.value = city.id;
        option.text = city.name + ", " + city.province;
        option.dataset.lat = city.lat;
        option.dataset.lon = city.lon;
        list.appendChild(option);
    });
}

async function detectLocation(){
    const response = await fetch("/detectlocation");
    if(!response.ok){
        alert("Unable to determine location");
        return;
    }
    loadNearbyCities();
}

async function selectCity(){
    const list = document.getElementById("citySelect");
    if(list.selectedIndex < 0)
        return;
    const cityID = list.value;
    await fetch("/setcity?id=" + cityID);
    updateSettings();
}

async function saveLocation(){
    const list = document.getElementById("citySelect");
    if(list.selectedIndex < 0)
        return;
    const id = list.value;
    const response = await fetch("/setcity?id=" + id);

    if(response.ok)
        alert("Location Saved");
}
// Auto refresh every 2 seconds on settings page
setInterval(updateSettings, 1000);
window.onload = () => {
    updateSettings();
    loadNearbyCities();
};