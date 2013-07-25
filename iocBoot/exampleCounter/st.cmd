< envPaths

cd ${TOP}

## Register all support components
dbLoadDatabase("dbd/exampleCounter.dbd")
exampleCounter_registerRecordDeviceDriver(pdbbase)

## Load record instances
dbLoadRecords("db/ai.db","name=double01")

cd ${TOP}/iocBoot/${IOC}
iocInit()
epicsThreadSleep(2.0)
casr
exampleCounterCreateRecord exampleCounter
startPVAServer
