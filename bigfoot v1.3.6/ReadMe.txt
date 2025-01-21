-- description

Bigfoot is an application that automatically launches other applications after the computer startup. 
It is realized through schedules. Actually, the real-time can differ from what is written in the schedule 
on each Windows system because the schedule is launched after the system loads all the necessary resources 
for its operation.

-- button functions

SORT - sorting items by time (ASC)
CLEAR - clear schedule
CHECK - checking if the location of previously added item in schedule was not changed
LAUNCH - launching selected item
ADD - adding selected item
DELETE - deleting selected item
VIEW MODE - restricts user actions on the current schedule(SORT, CLEAR, CHECK, LAUNCH, ADD, DELETE)
EDIT MODE - remove restrictions

File -> Exit - close application
File -> Open Schedule - opens schedule
File -> Save Schedule - save current schedule
File -> Close Schedule - closes current schedule

-- notes 

-bigfoot always opens in View mode 
-before saving schedule make shure that you have at least one item in it
-to create new schedule you should close the current schedule if it is opened
-if new schedule is not saved through button File -> Save Schedule and you pressed on File -> Open Schedule / File -> Exit 
you will get confirmation box if you want to save schedule. The cross button and File -> Close Schedule don't call 
confirmation box.
-after modifications in existing schedule item, the field in which they were done should lose focus before saving
-max 99 items