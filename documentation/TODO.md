TODO
===========

monitorPlugin
-------------

A debate is on-going about what semantics should be.


memory leak
--------

arrayPerformanceMain shows a slight memory leak at termination.

channel destroy and recreate
-------------------

longArrayGet and longArrayPut fail if the channel is destroyed and immediately recreated. If epicsThreadSleep(1.0) is called between destroy and recreate then they work. The current version of each does wait.

create more regresion tests
----------------

Currently only some simple tests exist. Most of the testing has been via the examples
