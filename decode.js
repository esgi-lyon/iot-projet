function decodeUplink(input) {
	var hex = String.fromCharCode(...Array.from(input.bytes)).split(' ')

	// Gently stolen from Antoine le boss
	const c02Bytes = hex[1];
	const c02Binary = parseInt(c02Bytes, 16);
  
	const tempBytes = hex[2];
	const tempBinary = parseInt(tempBytes, 16) * 0.1 - 40 ;
  
	const humidityBytes = hex[3];
	const humidityBinary = parseInt(humidityBytes, 16) * 0.39;
  
	return {
	  data: {
		co2: c02Binary,
		temperature: tempBinary,
		humidity: humidityBinary
	  },
	  warnings: [],
	  errors: []
	}
}

export default decodeUplink