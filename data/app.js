function updateStatus() {
	fetch('/status')
	.then(response => {
		if (!response.ok) throw new Error("HTTP error " + response.status);
		return response.json();
	})
	.then(data => {
		//if (!data) return;

        // Main page elements (safe)
		//document.getElementById("led").innerHTML = data.led ? "ON" : "OFF";
		var uptimeEl = document.getElementById("uptime");
        if (uptimeEl) uptimeEl.innerHTML = data.uptime || "";
		//document.getElementById("uptime").innerHTML = data.uptime;
		//document.getElementById("clients").innerHTML = data.clients;

		let err1 = document.getElementById("bat1error");
		if(data.battery1_error && data.battery1_error.length > 0){
			err1.innerHTML = data.battery1_error;
			err1.style.display = "block";
		}else{
			err1.style.display = "none";
		}

		let dot1 = document.getElementById("bat1dot");
		if(data.battery1_connected){
			dot1.className = "status-dot online";
		}else{
			dot1.className = "status-dot offline";
		}

		document.getElementById("bat1soc").innerHTML = "SOC: " + data.battery1_soc + "%";
		let soc1 = data.battery1_soc;
		document.getElementById("bat1soc").style.color = soc1 > 50 ? "white" : "#c0392b";
		document.getElementById("bat1socbar").style.width = soc1 + "%";
		document.getElementById("bat1socbar").style.background = soc1 > 50 ? "#27ae60" : soc1 > 20 ? "#f39c12" : "#f70202";

		document.getElementById("bat1volt").innerHTML = data.battery1_voltage + " V";
		document.getElementById("bat1current").innerHTML = data.battery1_current + " A";
		let bat1amp = data.battery1_current;
		document.getElementById("bat1current").style.color = bat1amp > 0 ? "green" : bat1amp < 0 ? "#f70202" : "black";
		document.getElementById("bat1temp").innerHTML = data.battery1_temp + " °C";
		
		document.getElementById("bat1age").innerHTML = data.battery1_age < 0 ? "Never" : data.battery1_age + " sec";

		let err2 = document.getElementById("bat2error");
		if(data.battery2_error && data.battery2_error.length > 0){
			err2.innerHTML = data.battery2_error;
			err2.style.display = "block";
		}else{
			err2.style.display = "none";
		}

		let dot2 = document.getElementById("bat2dot");
		if(data.battery2_connected){
			dot2.className = "status-dot online";
		}else{
			dot2.className = "status-dot offline";
		}

		document.getElementById("bat2soc").innerHTML = "SOC: " + data.battery2_soc + "%";
		let soc2 = data.battery2_soc;
		document.getElementById("bat2soc").style.color = soc2 > 50 ? "white" : "#c0392b";
		document.getElementById("bat2socbar").style.width = soc2 + "%";
		document.getElementById("bat2socbar").style.background = soc2 > 50 ? "#27ae60" : soc2 > 20 ? "#f39c12" : "#f70202";

		document.getElementById("bat2volt").innerHTML = data.battery2_voltage + " V";
		document.getElementById("bat2current").innerHTML = data.battery2_current + " A";
		let bat2amp = data.battery2_current;
		document.getElementById("bat2current").style.color = bat2amp > 0 ? "#27ae60" : bat2amp < 0 ? "#f70202" : "black";
		document.getElementById("bat2temp").innerHTML = data.battery2_temp + " °C";
		
		document.getElementById("bat2age").innerHTML = data.battery2_age < 0 ? "Never" : data.battery2_age + " sec";
		
		// === Cell values - only update if they exist on the page ===
        let cellElements = ["battery1_cell1", "battery1_cell2", "battery1_cell3", "battery1_cell4", "battery1_cellDiff", "celldelta1",
                           "battery2_cell1", "battery2_cell2", "battery2_cell3", "battery2_cell4", "battery2_cellDiff", "celldelta2"];

        cellElements.forEach(id => {
            let el = document.getElementById(id);
            if (el && data[id] !== undefined) {
                el.innerHTML = data[id] + " V";
            }
        });

		document.getElementById("totalvolt").innerHTML = data.total_voltage;
		document.getElementById("totalcurrent").innerHTML = data.total_current;
		let totamp = data.total_current;
		document.getElementById("totalcurrent").style.color = totamp > 0 ? "#27ae60" : totamp < 0 ? "#f70202" : "black";
		document.getElementById("totalpower").innerHTML = data.total_watts;
		let totwatt = data.total_watts;
		document.getElementById("totalpower").style.color = totwatt > 0 ? "#27ae60" : totwatt < 0 ? "#f70202" : "black";
		document.getElementById("totaltemp").innerHTML = data.total_temp;
		document.getElementById("totalsoc").innerHTML =	"SOC: " + data.total_soc + "%";
		document.getElementById("totalsocbar").style.width = data.total_soc + "%";
		document.getElementById("totaltemp").innerHTML = data.total_temp;
		document.getElementById("remainingah").innerHTML = data.total_remaining_ah;
		document.getElementById("capacityah").innerHTML = data.total_capacity_ah;

    });
    // .catch(err => console.error("UpdateStatus update error:", err));
}

function toggleBatteryCells(panelId, batteryNum) {
	let div = document.getElementById(panelId);
	let isVisible = div.style.display === "none";

	div.style.display = isVisible ? "block" : "none";
	
	// Dynamic fetch URL based on the battery number
	fetch(`/cells?visible=${isVisible ? 1 : 0}`);
	fetch(`/cells${batteryNum}?visible=${isVisible ? 1 : 0}`);
}

setInterval(updateStatus, 1000);
window.onload = updateStatus;


