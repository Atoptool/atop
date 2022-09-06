/*
 ** This is a JavaScript for ATOP Http Server
 ** 
 ** ==========================================================================
 ** Author:      Enhua Zhou
 ** E-mail:      zhouenhua@bytedance.com
 ** Date:        September 2022
 ** --------------------------------------------------------------------------
 ** Copyright (C) 2022 Enhua Zhou <zhouenhua@bytedance.com>
 **
 ** This program is free software; you can redistribute it and/or modify it
 ** under the terms of the GNU General Public License as published by the
 ** Free Software Foundation; either version 2, or (at your option) any
 ** later version.
 **
 ** This program is distributed in the hope that it will be useful, but
 ** WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 ** See the GNU General Public License for more details.
 **
 ** You should have received a copy of the GNU General Public License
 ** along with this program; if not, write to the Free Software
 ** Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 ** --------------------------------------------------------------------------
 */

const OrderType = {
    CPU: 'CPU',
    DISK: 'DISK',
    MEM: 'MEM',
}
Object.freeze(OrderType);

const TempleteType = {
    CommandLine: 'command_line',
    Disk: 'disk',
    Generic: 'generic',
    Memory: 'memory'
}
Object.freeze(TempleteType);

const DEFAULT_PROC_SHOW_NUM = 200;

const DEFAULT_CPU_SHOW_NUM = 0;
const DEFAULT_GPU_SHOW_NUM = 2;
const DEFAULT_DISK_SHOW_NUM = 1;
const DEFAULT_INTERFACE_SHOW_NUM = 2;
const DEFAULT_INFINIBAND_SHOW_NUM = 2;
const DEFAULT_NFS_SHOW_NUM = 2;
const DEFAULT_CONTAINER_SHOW_NUM = 1;
const DEFAULT_NUMA_SHOW_NUM = 0;
const DEFAULT_LLC_SHOW_NUM = 0;

var proc_show_num = DEFAULT_PROC_SHOW_NUM;
var cpu_show_num = DEFAULT_CPU_SHOW_NUM;
var gpu_show_num = DEFAULT_GPU_SHOW_NUM;
var disk_show_num = DEFAULT_DISK_SHOW_NUM;
var interface_show_num = DEFAULT_INTERFACE_SHOW_NUM;
var infiniband_show_num = DEFAULT_INFINIBAND_SHOW_NUM;
var nfs_show_num = DEFAULT_NFS_SHOW_NUM;
var container_show_num = DEFAULT_CONTAINER_SHOW_NUM;
var numa_show_num = DEFAULT_NUMA_SHOW_NUM;
var LLC_show_num = DEFAULT_LLC_SHOW_NUM;

// second
var timestamp_sample = Date.parse(new Date()) / 1000;

var template_type = TempleteType.Generic;
var order_type = OrderType.CPU;
var only_proc = true;

window.onkeydown = function (event) {
    var keyCode = event.key;

    now = Date.parse(new Date()) / 1000;
    switch (keyCode) {
        case 'b':
            var now_date_str = new Date(timestamp_sample * 1000).format("YYYYMMDDhhmm");

            var chosed_date_str = prompt("Enter new time (format [YYYYMMDD]hhmm): ", now_date_str);
            if (chosed_date_str != null) {
                if (!parseDateToTimesample(chosed_date_str)) {
                    alert("Wrong time!")
                }
            }
            break;
        case 'T':
            if (timestamp_sample - 10 < now) {
                timestamp_sample = timestamp_sample - 10;
            }
            break;
        case 't':
            if (timestamp_sample + 10 < now) {
                timestamp_sample = timestamp_sample + 10;
            }
            break;
        case 'l':
            var new_process_show_num = prompt("Maxinum lines for process statistics (now " + proc_show_num + "): ", DEFAULT_PROC_SHOW_NUM);
            if (new_process_show_num != null) {
                proc_show_num = new_process_show_num;
            }

            var new_cpu_show_num = prompt("Maxinum lines for per-cpu statistics (now " + cpu_show_num + "): ", DEFAULT_CPU_SHOW_NUM);
            if (new_cpu_show_num != null) {
                cpu_show_num = new_cpu_show_num;
            }

            var new_gpu_show_num = prompt("Maxinum lines for per-gpu statistics (now " + gpu_show_num + "): ", DEFAULT_GPU_SHOW_NUM);
            if (new_gpu_show_num != null) {
                gpu_show_num = new_gpu_show_num;
            }

            var new_disk_show_num = prompt("Maxinum lines for disk statistics (now " + disk_show_num + "): ", DEFAULT_DISK_SHOW_NUM);
            if (new_disk_show_num != null) {
                disk_show_num = new_disk_show_num;
            }

            var new_interface_show_num = prompt("Maxinum lines for interface statistics (now " + interface_show_num + "): ", DEFAULT_INTERFACE_SHOW_NUM);
            if (new_interface_show_num != null) {
                interface_show_num = new_interface_show_num;
            }

            var new_infiniband_show_num = prompt("Maxinum lines for infiniband port statistics (now " + infiniband_show_num + "): ", DEFAULT_INFINIBAND_SHOW_NUM);
            if (new_infiniband_show_num != null) {
                infiniband_show_num = new_infiniband_show_num;
            }

            var new_nfs_show_num = prompt("Maxinum lines for infiniband port statistics (now " + nfs_show_num + "): ", DEFAULT_NFS_SHOW_NUM);
            if (new_nfs_show_num != null) {
                nfs_show_num = new_nfs_show_num;
            }

            var new_container_show_num = prompt("Maxinum lines for container statistics (now " + container_show_num + "): ", DEFAULT_CONTAINER_SHOW_NUM);
            if (new_container_show_num != null) {
                container_show_num = new_container_show_num;
            }

            var new_numa_show_num = prompt("Maxinum lines for numa statistics (now " + numa_show_num + "): ", DEFAULT_NUMA_SHOW_NUM);
            if (new_numa_show_num != null) {
                numa_show_num = new_numa_show_num;
            }

            var new_LLC_show_num = prompt("Maxinum lines for LLC statistics (now " + LLC_show_num + "): ", DEFAULT_LLC_SHOW_NUM);
            if (new_LLC_show_num != null) {
                LLC_show_num = new_LLC_show_num;
            }

            break;
        case 'g':
            order_type = OrderType.CPU;
            template_type = TempleteType.Generic;
            break;
        case 'm':
            order_type = OrderType.MEM;
            template_type = TempleteType.Memory;
            break;
        case 'd':
            order_type = OrderType.DISK;
            template_type = TempleteType.Disk;
            break;
        case 'c':
            order_type = OrderType.DISK;
            template_type = TempleteType.CommandLine;
            break;
        case 'y':
            only_proc = !only_proc;
            break;
        default:
            return
    }

    main();
}

window.onload = function () {
    main();
}

function main() {
    host = document.location.host;
    getHtmlTemplateFromServer(getJsonRawDataFromServer, host);
}

function getHtmlTemplateFromServer(getJsonDataFunction, host) {
    var url = "http://" + host + "/template?type=" + template_type;
    var request = new XMLHttpRequest();

    request.open("GET", url, true);
    request.send();

    request.onload = function () {
        if (request.status === 200) {
            rawHtmlTemplate = request.responseText;
            getJsonDataFunction(LoadHtmlFromJson, rawHtmlTemplate, host);
            return;
        }
    }
}

function getJsonRawDataFromServer(loadFunction, templateHtml, host) {
    var url = "http://" + host + "/showsamp?timestamp=" + timestamp_sample + "&lables=ALL";
    var request = new XMLHttpRequest();

    request.open("GET", url, true);
    request.send();

    request.onload = function () {
        if (request.status === 200) {
            rawData = request.responseText;
            loadFunction(rawData, templateHtml);
            return;
        } else {
            alert("wrong timestamp or other error!");
            return;
        }
    }
}

function LoadHtmlFromJson(rawJson, template) {
    var parser = new DOMParser();
    doc = parser.parseFromString(template, "text/html");

    let tpl = doc.getElementById('tpl_general').innerHTML;
    let node = HtmlToNode(tpl);

    document.getElementById('target').innerHTML = ParseJsonToHtml(node, rawJson);
}

function ParseJsonToHtml(node, rawData) {
    var nestJson = "[" + rawData + "]";
    var json = JSON.parse(nestJson);
    preprocessJson(json);

    // sec to us
    percputot = json[0]["EXTRA"]["percputot"];
    hertz = json[0]["CPU"]["hertz"];
    return repeatedNode(node, json, percputot, hertz);
}

function preprocessJson(json) {
    parseAtopHeader(json);
    parseAtopProcess(json);
}

function parseAtopHeader(json) {
    timestamp = json[0]["timestamp"];

    //date
    var now = new Date(timestamp * 1000);
    y = now.getFullYear();
    m = now.getMonth() + 1;
    d = now.getDate();
    x = y + "-" + (m < 10 ? "0" + m : m) + "-" + (d < 10 ? "0" + d : d) + " " + now.toTimeString().slice(0, 8);

    json[0]["date"] = x;

    elapsed = json[0]["elapsed"];

    //CPU line
    CPUEntry = json[0]["CPU"];
    CPUtot = CPUEntry["stime"] + CPUEntry["utime"] + CPUEntry["ntime"] +
        CPUEntry["itime"] + CPUEntry["wtime"] + CPUEntry["Itime"] +
        CPUEntry["Stime"] + CPUEntry["steal"];

    if (CPUtot === 0) {
        CPUtot = 1;
    }

    perCPUtot = CPUtot / CPUEntry["nrcpu"];
    mstot = CPUtot * 1000 / CPUEntry["hertz"] / CPUEntry["nrcpu"];

    json[0]["CPU"]["cpubusy"] = ((CPUtot - CPUEntry["itime"] - CPUEntry["wtime"]) / perCPUtot * 100).toFixed(1);

    //cpu line
    cpulist = json[0]["cpu"];
    cpulist.sort(compareOrderByCpu);

    if (cpu_show_num > cpulist.length) {
        cpu_show_num = cpulist.length;
    }
    json[0]["cpu"] = cpulist.slice(0, cpu_show_num);

    //GPU line
    //TODO

    //CPL line
    json[0]["CPL"]["nrcpu"] = CPUEntry["nrcpu"];

    //MEM line

    //SWP line

    //NUM line
    json[0]["MEM"]["numanode"] = json[0]["NUM"].length;

    numalist = json[0]["NUM"];
    numalist.forEach(function (numanode, index) {
        numanode["numanodeId"] = index;
    })
    if (numa_show_num > numalist.length) {
        numa_show_num = numalist.length;
    }
    json[0]["NUM"] = numalist.slice(0, numa_show_num);

    //PAG line

    //PSI line
    psiEntry = json[0]["PSI"];
    json[0]["PSI"]["cpusome"] = ((psiEntry["cstot"] / elapsed * 100) > 100) ? 100 : psiEntry["cstot"] / (elapsed * 10000);
    json[0]["PSI"]["iosome"] = ((psiEntry["iostot"] / elapsed * 100) > 100) ? 100 : psiEntry["iostot"] / (elapsed * 10000);
    json[0]["PSI"]["iofull"] = ((psiEntry["ioftot"] / elapsed * 100) > 100) ? 100 : psiEntry["ioftot"] / (elapsed * 10000);
    json[0]["PSI"]["memsome"] = ((psiEntry["mstot"] / elapsed * 100) > 100) ? 100 : psiEntry["mstot"] / (elapsed * 10000);
    json[0]["PSI"]["memfull"] = ((psiEntry["mftot"] / elapsed * 100) > 100) ? 100 : psiEntry["mftot"] / (elapsed * 10000);
    json[0]["PSI"]["cs"] = psiEntry["cs10"].toFixed(0) + "/" + psiEntry["cs60"].toFixed(0) + "/" + psiEntry["cs300"].toFixed(0);
    json[0]["PSI"]["ms"] = psiEntry["ms10"].toFixed(0) + "/" + psiEntry["ms60"].toFixed(0) + "/" + psiEntry["ms300"].toFixed(0);
    json[0]["PSI"]["is"] = psiEntry["ios10"].toFixed(0) + "/" + psiEntry["ios60"].toFixed(0) + "/" + psiEntry["ios300"].toFixed(0);

    //DSK line
    dsklist = json[0]["DSK"];
    dsklist.forEach(function (dsk, index) {
        iotot = dsk["nread"] + dsk["nwrite"];
        dsk["avio"] = (iotot ? dsk["io_ms"] / iotot : 0.0).toFixed(2);
        dsk["diskbusy"] = (mstot ? dsk["io_ms"] / mstot : 0).toFixed(2);
        dsk["MBr/s"] = (dsk["nrsect"] / 2 / 1024 / elapsed).toFixed(1);
        dsk["MBw/s"] = (dsk["nwsect"] / 2 / 1024 / elapsed).toFixed(1);
    })
    dsklist.sort(compareOrderByDiskIoms);
    if (disk_show_num > dsklist.length) {
        disk_show_num = dsklist.length;
    }
    json[0]["DSK"] = dsklist.slice(0, disk_show_num);

    //NET line
    netlist = json[0]["NET"];
    netlist.sort(compareOrderByNet);
    if (interface_show_num > netlist.length) {
        interface_show_num = netlist.length
    }
    json[0]["NET"] = netlist.slice(0, interface_show_num);

    //IFB line
    // TODO

    //LLC line
    // TODO

    // EXTRA
    extra = new Object();
    extra["mstot"] = mstot;
    extra["percputot"] = perCPUtot;

    extra["availmem"] = json[0]["MEM"]["physmem"] / 1024;
    json[0]["EXTRA"] = extra;
}

function parseAtopProcess(json) {
    percputot = json[0]["EXTRA"]["percputot"];
    availmem = json[0]["EXTRA"]["availmem"];

    prc = json[0]["PRC"];
    prm = json[0]["PRM"];
    prd = json[0]["PRD"];
    prg = json[0]["PRG"];

    prcmap = ArrayToMap(prc);
    prmmap = ArrayToMap(prm);
    prdmap = ArrayToMap(prd);
    prgmap = ArrayToMap(prg);

    hprc = new Object();
    hprc["proc_count"] = 0;
    hprc["sleep_count"] = 0;
    hprc["zombie_count"] = 0;
    hprc["exit_count"] = 0;
    hprc["running_count"] = 0;
    hprc["sleep_interrupt_count"] = 0;
    hprc["sleep_uninterrupt_count"] = 0;

    hprc["stime_unit_time"] = 0;
    hprc["utime_unit_time"] = 0;

    prgmap.forEach(function (iprg, index) {
        if (iprg["state"] == "E") {
            hprc["exit_count"]++;
        } else {
            if (iprg["state"] == "Z") {
                hprc["zombie_count"]++;
            }
            hprc["sleep_interrupt_count"] += iprg["nthrslpi"];
            hprc["sleep_uninterrupt_count"] += iprg["nthrslpu"];
            hprc["running_count"] += iprg["nthrrun"];
            hprc["proc_count"]++;
        }
    })

    // get availdsk
    var availdisk = 0;
    prdmap.forEach(function (iprd, index) {
        var nett_wsz = 0;
        if (iprd["wsz"] > iprd["cwsz"]) {
            nett_wsz = iprd["wsz"] - iprd["cwsz"];
        } else {
            nett_wsz = 0;
        }

        availdisk += iprd["rsz"];
        availdisk += nett_wsz;
    })

    aggregate_pr_map = new Map();
    prcmap.forEach(function (iprc, index) {
        // proc_count not contains exit proc

        iprm = prmmap.get(index);
        iprd = prdmap.get(index);
        iprg = prgmap.get(index);

        if (iprg["isproc"] !== 1) {
            iprg["tid"] = iprg["tgid"];
            if (only_proc) {
                return;
            }
        } else {
            iprg["tid"] = '-';
        }

        hprc["stime_unit_time"] += iprc["stime"];
        hprc["utime_unit_time"] += iprc["utime"];

        //cpu
        cputot = iprc["stime"] + iprc["utime"];
        iprc["cpubusy"] = ((cputot / percputot) * 100).toFixed(1);

        iprc["stime_unit_time"] = iprc["stime"];
        iprc["utime_unit_time"] = iprc["utime"];
        iprc["curcpu"] = iprc["curcpu"] ? iprc["curcpu"] : "-";

        //general
        pro_name = iprg["name"].replace(/[()]/ig, "");
        iprg["name"] = pro_name.length > 15 ? pro_name.substr(0, 15) : pro_name;
        cmdline = iprg["cmdline"].replace(/[()]/ig, "");
        if (cmdline.length == 0) {
            iprg["cmdline"] = iprg["name"]
        } else {
            iprg["cmdline"] = cmdline
        }
        iprg["exitcode"] = iprg["exitcode"] ? iprg["exitcode"] : "-";

        //mem
        membusy = (iprm["rmem"] * 100 / availmem).toFixed(1)
        if (membusy > 100) {
            iprm["membusy"] = 100;
        } else {
            iprm["membusy"] = membusy;
        }

        iprm["vexec"] = iprm["vexec"] * 1024;
        iprm["vlibs"] = iprm["vlibs"] * 1024;
        iprm["vdata"] = iprm["vdata"] * 1024;
        iprm["vstack"] = iprm["vstack"] * 1024;
        iprm["vlock"] = iprm["vlock"] * 1024;
        iprm["vmem"] = iprm["vmem"] * 1024;
        iprm["rmem"] = iprm["rmem"] * 1024;
        iprm["pmem"] = iprm["pmem"] * 1024;
        iprm["vgrow"] = iprm["vgrow"] * 1024;
        iprm["rgrow"] = iprm["rgrow"] * 1024;
        iprm["vswap"] = iprm["vswap"] * 1024;

        //disk
        var nett_wsz = 0;
        if (iprd["wsz"] > iprd["cwsz"]) {
            nett_wsz = iprd["wsz"] - iprd["cwsz"];
        } else {
            nett_wsz = 0;
        }

        diskbusy = ((iprd["rsz"] + nett_wsz) * 100 / availdisk).toFixed(1)
        if (diskbusy > 100) {
            iprd["diskbusy"] = 100;
        } else {
            iprd["diskbusy"] = diskbusy;
        }

        iprd["rsz"] = iprd["rsz"] * 512;
        iprd["wsz"] = iprd["wsz"] * 512;
        iprd["cwsz"] = iprd["cwsz"] * 512;

        aggregate_pr = Object.assign(iprc, iprm, iprd, iprg);

        aggregate_pr_map.set(index, aggregate_pr);
    })

    // map to arr for sort
    aggregate_pr_arr = [];
    aggregate_pr_map.forEach(function (value, key) {
        aggregate_pr_arr.push(value);
    })

    switch (order_type) {
        case OrderType.CPU:
            aggregate_pr_arr.sort(compareOrderByCpu);
            break;
        case OrderType.DISK:
            aggregate_pr_arr.sort(compareOrderByDisk);
            break;
        case OrderType.MEM:
            aggregate_pr_arr.sort(compareOrderByMem);
            break;
        default:
            aggregate_pr_arr.sort(compareOrderByCpu);
    }

    if (proc_show_num > aggregate_pr_arr.length) {
        proc_show_num = aggregate_pr_arr.length;
    }

    json[0]["PRSUMMARY"] = aggregate_pr_arr.slice(0, proc_show_num);
    json[0]["HPRC"] = hprc;
    delete json[0]["PRC"];
    delete json[0]["PRM"];
    delete json[0]["PRD"];
    delete json[0]["PRG"];
}

function repeatedNode(node, arr, percputot) {
    let out = [];

    for (let i = 0; i < arr.length; i++) {
        let tmp = node.outerHTML;
        tmp = tmp.replace(/\s/g, ' ');

        let map = arr[i];
        // parse inner array
        for (let j in map) {
            if (map[j] instanceof Array) {
                let subNode = node.querySelector('.' + j);
                if (subNode) {
                    let subHTML = repeatedNode(subNode, map[j], percputot, hertz);
                    let subTpl = subNode.outerHTML.replace(/\s/g, ' ');
                    tmp = tmp.replace(subTpl, subHTML);
                }
            }
        }

        // parse Object
        for (let j in map) {
            if (map[j] instanceof Object && !(map[j] instanceof Array)) {
                let subNode = node.querySelector('.' + j);
                if (subNode) {
                    let subHTML = repeatedNode(subNode, [map[j]], percputot, hertz);
                    let subTpl = subNode.outerHTML.replace(/\s/g, ' ');
                    tmp = tmp.replace(subTpl, subHTML);
                }
            }
        }

        for (let j in map) {
            if (typeof map[j] === 'string' || typeof map[j] === 'number') {
                if (["stime", "utime", "ntime", "itime", "wtime", "Itime", "Stime", "steal", "guest"].indexOf(j) !== -1) {
                    map[j] = ParseCPUPercentValue(map[j], percputot);
                } else if (["cpusome", "memsome", "memfull", "iosome", "iofull"].indexOf(j) !== -1) {
                    map[j] = ParsePSIPercentValue(map[j]);
                } else if (["freq"].indexOf(j) !== -1) {
                    map[j] = ParseCPUCurfValue(map[j]);
                } else if (["stime_unit_time", "utime_unit_time", "rundelay", "blkdelay"].indexOf(j) !== -1) {
                    map[j] = ParseTimeValue(map[j], hertz, j);
                } else if (["buffermem", "cachedrt", "cachemem", "commitlim", "committed", "freemem", "freeswap",
                    "filepage", "physmem", "rgrow", "rsz", "shmrss", "slabmem", "swcac", "totmem", "totswap",
                    "vexec", "vdata", "vgrow", "vlibs", "vlock", "vmem", "vstack", "wsz", "rmem", "pmem", "vswap"].indexOf(j) !== -1) {
                    map[j] = ParseMemValue(map[j], j);
                } else if (["rbyte", "sbyte", "speed"].indexOf(j) !== -1) {
                    map[j] = ParseBandwidth(map[j], j);
                } else if (["minflt", "majflt"].indexOf(j) !== -1) {
                    map[j] = Value2ValueStr(map[j], j);
                } else if (["cpubusy", "membusy", "diskbusy"].indexOf(j) !== -1) {
                    map[j] = Value2ValuePercent(map[j], j);
                }
                else {
                    map[j] = map[j];
                }

                if (typeof map[j] === 'string') {
                    map[j] = map[j];
                }

                let re = new RegExp('{' + j + '}', 'g');
                tmp = tmp.replace(re, map[j]);
            }
        }

        out.push(tmp);
    }

    return out.join('');
}

function ParseCPUPercentValue(value, percputot) {
    return Math.round(value * 100 / percputot).toFixed(0) + "%";
}

function ParseCPUCurfValue(value) {
    if (value < 1000) {
        return value + "MHz";
    } else {
        fval = value / 1000;
        prefix = 'G';
        if (fval >= 1000) {
            prefix = 'T';
            fval = fval / 1000;
        }

        if (fval < 10) {
            return fval.toFixed(2) + prefix + "Hz";
        } else {
            if (fval < 100) {
                return fval.toFixed(1) + prefix + "Hz";
            } else {
                return fval.toFixed(0) + prefix + "Hz";
            }
        }
    }
}

function ParsePSIPercentValue(value) {
    return Math.round(value).toFixed(0) + "%";
}

function ParseTimeValue(value, hertz, indicatorName) {
    // here is ms
    value = parseInt(value * 1000 / hertz);

    if (value < 100000) {
        return parseInt(value / 1000) + "." + parseInt(value % 1000 / 10) + "s";
    } else {
        //ms to sec
        value = parseInt((value + 500) / 1000);
        if (value < 6000) {
            //sec to min
            return parseInt(value / 60) + "m" + (value % 60) + "s";
        } else {
            //min to hour
            value = parseInt((value + 30) / 60);
            if (value < 6000) {
                return parseInt(value / 60) + "h" + (value % 60) + "m";
            } else {
                // hour to day
                value = parseInt((value + 30) / 60);
                return parseInt(value / 24) + "d" + (value % 24) + "h";
            }
        }
    }
}

function ParseMemValue(ByteValue, indicatorName) {
    var unit = 1024; // byte
    ByteValue = ByteValue - 0;

    var prefix = "";
    if (ByteValue < 0) {
        ByteValue = -ByteValue * 10;
        var prefix = "-";
    }

    if (ByteValue < unit) {
        result = ByteValue + 'B';
    } else if (ByteValue < Math.pow(unit, 2)) {
        result = (ByteValue / unit).toFixed(1) + "KB";
    } else if (ByteValue < Math.pow(unit, 3)) {
        result = (ByteValue / Math.pow(unit, 2)).toFixed(1) + "MB";
    } else if (ByteValue < Math.pow(unit, 4)) {
        result = (ByteValue / Math.pow(unit, 3)).toFixed(1) + "G";
    } else {
        result = (ByteValue / Math.pow(unit, 4)).toFixed(1) + "T";
    }

    return prefix + result;
}

function ParseBandwidth(BandwidthValue, indicatorName) {
    var unit = 1000;

    if (BandwidthValue < unit) {
        return BandwidthValue + ' Kbps';
    } else if (BandwidthValue < Math.pow(unit, 2)) {
        return (BandwidthValue / unit).toFixed(2) + " Mbps";
    } else if (BandwidthValue < Math.pow(unit, 3)) {
        return (BandwidthValue / Math.pow(unit, 2)).toFixed(2) + " Gbps";
    } else {
        return (BandwidthValue / Math.pow(unit, 3)).toFixed(2) + " Tbps";
    }
}

// for number toooo long
function Value2ValueStr(value, indicatorName) {
    // TODO
    return value;
}

function Value2ValuePercent(value, indicatorName) {
    return value + "%";
}

function ArrayToMap(array) {
    map = new Map();
    for (i of array) {
        map.set(i["pid"], i);
    }
    return map;
}

function HtmlToNode(html) {
    let div = document.createElement('div');
    let pos = html.indexOf('<');
    div.innerHTML = html.substring(pos);
    return div.firstChild;
}

function compareOrderByCpu(a, b) {
    acpu = a["utime"] + a["stime"];
    bcpu = b["utime"] + b["stime"];
    if (acpu > bcpu) {
        return -1;
    }
    if (acpu < bcpu) {
        return 1;
    }
    return 0;
}

function compareOrderByDisk(a, b) {
    if (a["wsz"] > a["cwsz"]) {
        adsk = a["rio"] + a["wsz"] - a["cwsz"];
    } else {
        adsk = a["rio"];
    }

    if (b["wsz"] > b["cwsz"]) {
        bdsk = b["rio"] + b["wsz"] - b["cwsz"];
    } else {
        bdsk = b["rio"];
    }

    if (adsk > bdsk) {
        return -1;
    }

    if (adsk < bdsk) {
        return 1;
    }

    return compareOrderByCpu(a, b);
}

function compareOrderByDiskIoms(a, b) {
    adsk_value = a["io_ms"]
    bdsk_value = b["io_ms"]

    if (adsk_value > bdsk_value) {
        return -1;
    }

    if (adsk_value < bdsk_value) {
        return 1;
    }

    return 0;
}

function compareOrderByMem(a, b) {
    amem = a["rmem"];
    bmem = b["rmem"];

    if (amem > bmem) {
        return -1;
    }
    if (amem < bmem) {
        return 1;
    }

    return 0;
}

function compareOrderByNet(a, b) {
    anet_value = a["rpack"] + a["spack"]
    bnet_value = b["rpack"] + b["bpack"]

    if (anet_value > bnet_value) {
        return -1;
    }

    if (anet_value < bnet_value) {
        return 1;
    }

    return 0;
}

function parseDateToTimesample(itim) {
    ilen = itim.length
    if (ilen != 4 && ilen != 5 && ilen != 12 && ilen != 13) {
        return false;
    }

    var year, month, day, hour, minute

    if (ilen == 12 || ilen == 13) {
        year = itim.slice(0, 4);
        month = itim.slice(4, 6);
        day = itim.slice(6, 8);
        hour = itim.slice(8, 10);
        minute = itim.slice(-2);
    } else if (ilen == 4 || ilen == 5) {
        now_date = new Date();
        year = now_date.getFullYear();
        month = now_date.getMonth() + 1;
        day = now_date.getDate();

        hour = itim.slice(0, 2);
        minute = itim.slice(-2);
    }

    if (year < 100 || month < 0 || month > 11 || day < 1 || day > 31 || hour < 0 || hour > 23 || minute < 0 || minute > 59) {
        return false;
    }

    timestamp_sample = Date.parse(new Date(year, month - 1, day, hour, minute)) / 1000;
    return true;
}

Date.prototype.format = function (fmt) {
    let ret;
    const opt = {
        "Y+": this.getFullYear().toString(),
        "M+": (this.getMonth() + 1).toString(),
        "D+": this.getDate().toString(),
        "h+": this.getHours().toString(),
        "m+": this.getMinutes().toString(),
    };
    for (let k in opt) {
        ret = new RegExp("(" + k + ")").exec(fmt);
        if (ret) {
            fmt = fmt.replace(ret[1], (ret[1].length == 1) ? (opt[k]) : (opt[k].padStart(ret[1].length, "0")))
        };
    };
    return fmt;
}
