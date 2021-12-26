// REQUIRE ////////////////////////////////////////////////////////////////////

const path = require("path");
const fs = require("fs");
global.appRoot = path.join(__dirname, "..");

const stLinkProtCli = require(path.join(appRoot, "@stm32/stm32cubemonitor-stlink-protocol/src/stlinkprotocol")).cli;
const stLinkProt = require(path.join(appRoot, "@stm32/stm32cubemonitor-stlink-protocol/src/stlinkprotocol"));
const apidefs = require(path.join(appRoot, "@stm32/stm32cubemonitor-stlink-protocol/src/stlinkprotocol")).apidefs;

// CONST //////////////////////////////////////////////////////////////////////

const RAM_START = 0x20000000;
const RAM_START_SAFE = RAM_START + 0x100;
const RAM_END = 0x20008000;
const RAM_END_SAFE = RAM_END - 0x100;
const DWT_BASE = 0xE0001000;                            /*!< DWT Base Address */
const FLASH_START = 0x8000000;


// https://stackoverflow.com/questions/9781218/how-to-change-node-jss-console-font-color
const COLORS = {
    BLACK: "\x1b[30m",
    RED: "\x1b[31m",
    GREEN: "\x1b[32m",
    YELLOW: "\x1b[33m",
    BLUE: "\x1b[34m",
    MAGENTA: "\x1b[35m",
    CYAN: "\x1b[36m"
};

const logResult = (result, str) => {
    console.log(result ? COLORS.GREEN : COLORS.RED, result ? "PASS: " + str : "FAIL: " + str);
};

// VARIABLES //////////////////////////////////////////////////////////////////

let stConnection = {
    protocol: "SWD",
    frequency: "",
    deviceIndex: 0,                    // the identifier of a probe
    typeConnect: "p2p"
};

// HELPER FUNCTIONS ///////////////////////////////////////////////////////////

// https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Math/random
function getRandomInt(min, max) {
    min = Math.ceil(min);
    max = Math.floor(max);
    return Math.floor(Math.random() * (max - min) + min); // The maximum is exclusive and the minimum is inclusive
}

// PROBE DISCOVERY ////////////////////

let listprobe = stLinkProt.probeDiscovery(stConnection.typeConnect);
let nbDevices = stLinkProtCli.getNbDevices(stConnection.typeConnect);

if (nbDevices > 0) {
    listprobe.forEach(probe => {
        console.log("Device Info:");
        console.log(`        Device Name : ${probe.probeName}`);
        console.log(`        Device Id : ${probe.probeId}`);
        for (const [protocol, frequencies] of Object.entries(probe.property)) {
            console.log(`        Protocol : ${protocol} at following frequencies : ${frequencies.join()}.`);
        }
    });
} else {
    console.log("No devices found !");
    process.exit();
}

// CONNECT ////////////////////////////

// do not ask for re-discovery by default, to avoid side effects when multiple probes are connected
stConnection.frequency = listprobe[stConnection.deviceIndex].property.SWD[0];
let res = stLinkProt.connectToTarget(listprobe[stConnection.deviceIndex].probeId, stConnection.protocol, stConnection.frequency, false, stConnection.typeConnect);

// update number of found devices, in case it has been updated by connectToTarget call
nbDevices = stLinkProtCli.getNbDevices(stConnection.typeConnect);

if (res.result == apidefs.STLINK_OK) {
    stConnection.deviceDesc = res.deviceDescription;
    console.log("Connection to target " + listprobe[stConnection.deviceIndex].probeId + " successful");
    console.log("        MCU ID : 0x" + res.MCUID.toString(16).padStart(3, "0"));
    console.log("        MCU Name : " + res.MCUName);
} else {
    console.log("Connection to target " + listprobe[stConnection.deviceIndex].probeId + " failed : " + stLinkProtCli.errorString(res.result));
    process.exit();
}

// TEST ///////////////////////////////

function test_flash_read_32(binFile, testsCnt) {
    const flash = fs.readFileSync(path.join(appRoot, binFile));
    const flashSize = Math.floor(flash.length / 4) * 4;

    console.log(COLORS.BLACK, "TEST: test_flash_read_32, compare with binary flash");
    let read32 = stLinkProtCli.JTAG_ReadMemory32Bit_ex(stConnection.deviceIndex, FLASH_START, flashSize, stConnection.typeConnect);
    if (read32.result != apidefs.STLINK_OK) {
        logResult(false, "Can not read flash");
        return;
    }

    logResult(flash.equals(read32.data), `${binFile} FLASH 32 bit read`);

    console.log(COLORS.BLACK, "TEST: test_flash_read_32, compare with binary flash - random offset and size");

    // now run some random size reads 
    for (let index = 0; index < testsCnt; index++) {
        const size = Math.floor(getRandomInt(0, flashSize) / 4) * 4;
        const offset = Math.floor(getRandomInt(0, flashSize - size) / 4) * 4;
        let read32 = stLinkProtCli.JTAG_ReadMemory32Bit_ex(stConnection.deviceIndex, FLASH_START + offset, size, stConnection.typeConnect);
        if (read32.result != apidefs.STLINK_OK) {
            logResult(false, "Can not read flash");
            return;
        }

        let pass = flash.slice(offset, offset + size).equals(read32.data);
        logResult(pass, `FLASH 32 bit read address: 0x${(offset + RAM_START).toString(16)} size: ${size} flash read`);
    }
}

function test_ram_read_write_32(testsCnt) {

    console.log(COLORS.BLACK, "TEST: test_ram_read_write_32, write and read whole RAM");

    // generate random values    
    let size = RAM_END_SAFE - RAM_START_SAFE;
    let buf = Buffer.allocUnsafe(size);
    buf.forEach((currentValue, index) => {
        buf[index] = getRandomInt(0, 256);
    });

    // write it
    let write32 = stLinkProtCli.JTAG_WriteMemory32Bit_ex(stConnection.deviceIndex, RAM_START_SAFE, size, buf, stConnection.typeConnect);
    if (write32.result != apidefs.STLINK_OK) {
        logResult(false, "Can not write RAM");
        return;
    }

    // read it
    let read32 = stLinkProtCli.JTAG_ReadMemory32Bit_ex(stConnection.deviceIndex, RAM_START_SAFE, size, stConnection.typeConnect);
    if (read32.result != apidefs.STLINK_OK) {
        logResult(false, "Can not read RAM");
        return;
    }

    logResult(buf.equals(read32.data), "RAM 32 bit write/read");

    console.log(COLORS.BLACK, "TEST: test_ram_read_write_32, write/read random RAM areas");

    // now run some random size write/reads 
    for (let index = 0; index < testsCnt; index++) {
        let size = Math.floor(getRandomInt(0, RAM_END_SAFE - RAM_START_SAFE) / 4) * 4;
        let offset = Math.floor(getRandomInt(0, RAM_END_SAFE - RAM_START_SAFE - size) / 4) * 4;

        let buf = Buffer.allocUnsafe(size);
        buf.forEach((currentValue, index) => {
            buf[index] = getRandomInt(0, 256);
        });

        // write it
        let write32 = stLinkProtCli.JTAG_WriteMemory32Bit_ex(stConnection.deviceIndex, RAM_START_SAFE + offset, size, buf, stConnection.typeConnect);
        if (write32.result != apidefs.STLINK_OK) {
            logResult(false, "Can not write RAM");
            return;
        }

        // read it
        let read32 = stLinkProtCli.JTAG_ReadMemory32Bit_ex(stConnection.deviceIndex, RAM_START_SAFE + offset, size, stConnection.typeConnect);
        if (read32.result != apidefs.STLINK_OK) {
            logResult(false, "Can not read RAM");
            return;
        }

        logResult(buf.equals(read32.data), `RAM 32 bit write/read address: 0x${(offset + RAM_START).toString(16)} and size: ${size}`);
    }

}

function test_flash_read(binFile, testsCnt) {
    const flash = fs.readFileSync(path.join(appRoot, binFile));

    console.log(COLORS.BLACK, "TEST: test_flash_read, compare with binary flash");
    let read32 = stLinkProt.readMemory(stConnection.deviceIndex, FLASH_START, flash.length, stConnection.typeConnect);
    if (read32.result != apidefs.STLINK_OK) {
        logResult(false, "Can not read flash");
        return;
    }

    logResult(flash.equals(read32.data), `${binFile} FLASH read`);

    console.log(COLORS.BLACK, "TEST: test_flash_read, compare with binary flash - random offset and size");

    // now run some random size reads 
    for (let index = 0; index < testsCnt; index++) {
        const size = getRandomInt(0, flash.length);
        const offset = getRandomInt(0, flash.length - size);
        let read32 = stLinkProt.readMemory(stConnection.deviceIndex, FLASH_START + offset, size, stConnection.typeConnect);
        if (read32.result != apidefs.STLINK_OK) {
            logResult(false, "Can not read flash");
            return;
        }

        let pass = flash.slice(offset, offset + size).equals(read32.data);
        logResult(pass, `FLASH read address: 0x${(offset + RAM_START).toString(16)} size: ${size} flash read`);
    }
}

function test_ram_read_write(testsCnt) {

    console.log(COLORS.BLACK, "TEST: test_ram_read_write, write and read whole RAM");

    // generate random values    
    let size = RAM_END_SAFE - RAM_START_SAFE;
    let buf = Buffer.allocUnsafe(size);
    buf.forEach((currentValue, index) => {
        buf[index] = getRandomInt(0, 256);
    });

    // write it
    let write32 = stLinkProt.writeMemory(stConnection.deviceIndex, RAM_START_SAFE, size, buf, stConnection.typeConnect);
    if (write32.result != apidefs.STLINK_OK) {
        logResult(false, "Can not write RAM");
        return;
    }

    // read it
    let read32 = stLinkProt.readMemory(stConnection.deviceIndex, RAM_START_SAFE, size, stConnection.typeConnect);
    if (read32.result != apidefs.STLINK_OK) {
        logResult(false, "Can not read RAM");
        return;
    }

    logResult(buf.equals(read32.data), "RAM write/read");

    console.log(COLORS.BLACK, "TEST: test_ram_read_write, write/read random RAM areas");

    // now run some random size write/reads 
    for (let index = 0; index < testsCnt; index++) {
        let size = getRandomInt(0, RAM_END_SAFE - RAM_START_SAFE);
        let offset = getRandomInt(0, RAM_END_SAFE - RAM_START_SAFE - size);

        let buf = Buffer.allocUnsafe(size);
        buf.forEach((currentValue, index) => {
            buf[index] = getRandomInt(0, 256);
        });

        // write it
        let write32 = stLinkProt.writeMemory(stConnection.deviceIndex, RAM_START_SAFE + offset, size, buf, stConnection.typeConnect);
        if (write32.result != apidefs.STLINK_OK) {
            logResult(false, "Can not write RAM");
            return;
        }

        // read it
        let read32 = stLinkProt.readMemory(stConnection.deviceIndex, RAM_START_SAFE + offset, size, stConnection.typeConnect);
        if (read32.result != apidefs.STLINK_OK) {
            logResult(false, "Can not read RAM");
            return;
        }

        logResult(buf.equals(read32.data), `RAM write/read address: 0x${(offset + RAM_START).toString(16)} and size: ${size}`);
    }
}

function test_speed(testsCnt) {
    let buf = Buffer.allocUnsafe(16);

    let timeName = `${testsCnt}-loop`;
    console.time(timeName);

    for (let index = 0; index < testsCnt; index++) {
        // write it
        let write32 = stLinkProt.writeMemory(stConnection.deviceIndex, RAM_START_SAFE, buf.length, buf, stConnection.typeConnect);
        if (write32.result != apidefs.STLINK_OK) {
            logResult(false, "Can not write RAM");
            return;
        }

        // read it
        let read32 = stLinkProt.readMemory(stConnection.deviceIndex, RAM_START_SAFE, 1024, stConnection.typeConnect);
        if (read32.result != apidefs.STLINK_OK) {
            logResult(false, "Can not read RAM");
            return;
        }
    }

    console.timeEnd(timeName);
}

test_flash_read_32("./test/app_g431_nuc32/Debug/app_g431_nuc32.bin", 10);
test_ram_read_write_32(10);
test_flash_read("./test/app_g431_nuc32/Debug/app_g431_nuc32.bin", 10);
test_ram_read_write(10);

test_speed(20);

// CLOSE //////////////////////////////
stLinkProt.closeProbes("", stConnection.typeConnect);