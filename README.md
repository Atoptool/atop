[atop](http://www.atoptool.nl)
==============================

Created/maintained by Gerlof Langeveld <gerlof.langeveld@atoptool.nl>

## Introduction
<img align="right" width="100" height="116" src="http://www.atoptool.nl/images/atoplogo.png">


Atop is an ASCII full-screen performance monitor for Linux that is capable
of reporting the activity of all processes (even if processes have finished
during the interval), daily logging of system and process activity for
long-term analysis, highlighting overloaded system resources by using colors,
etcetera. At regular intervals, it shows system-level activity related to the
CPU, memory, swap, disks (including LVM) and network layers, and for every
process (and thread) it shows e.g. the CPU utilization, memory growth,
disk utilization, priority, username, state, and exit code.
In combination with the optional kernel module *netatop*,
it even shows network activity per process/thread. 
In combination with the optional daemon *atopgpud*,
it also shows GPU activity on system level and process level. 

## Highlights

The command atop has some major advantages compared to other performance monitoring tools:

* __Resource consumption by *all* processes.__
It shows the resource consumption by all processes that were active during the interval, so also the resource consumption by those processes that have finished during the interval.

* __Utilization of *all* relevant resources.__
Obviously it shows system-level counters concerning utilization of cpu and memory/swap, however it also shows disk I/O and network utilization counters on system level.

* __Permanent logging of resource utilization.__
It is able to store raw counters in a file for long-term analysis on system level and process level. These raw counters are compressed at the moment of writing to minimize disk space usage. By default, the daily logfiles are preserved for 28 days.
System activity reports can be generated from a logfile by using the atopsar command.

* __Highlight critical resources.__
It highlights resources that have (almost) reached a critical load by using colors for the system statistics.

* __Scalable window width.__
It is able to add or remove columns dynamically at the moment that you enlarge or shrink the width of your window.

* __Resource consumption by individual *threads*.__
It is able to show the resource consumption for each thread within a process.

* __Watch activity only.__
By default, it only shows system resources and processes that were really active during the last interval, so output related to resources or processes that were completely passive during the interval is by default suppressed.

* __Watch deviations only.__
For the active system resources and processes, only the load during the last interval is shown (not the accumulated utilization since system boot or process startup).

* __Accumulated process activity per user.__
For each interval, it is able to accumulate the resource consumption for all processes per user.

* __Accumulated process activity per program.__
For each interval, it is able to accumulate the resource consumption for all processes with the same name.

* __Accumulated process activity per container.__
For each interval, it is able to accumulate the resource consumption for all processes within the same container.

* __Network activity per process.__
In combination with the optional kernel module netatop, it shows process-level counters concerning the number of TCP and UDP packets transferred, and the consumed network bandwidth per process.

* __GPU activity on system level and per process.__
In combination with the optional daemon atopgpud, it shows system-level and process-level counters concerning the load and memory utilization per GPU.
