import decodeUplink from "./decode.js";

// 48 48 32 50 50 32 51 48 48 32 50 48
const test = { bytes: new TextEncoder().encode("00 22 300 20") }

console.log(test);

console.log(decodeUplink(test));
