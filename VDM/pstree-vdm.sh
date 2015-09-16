#!/bin/bash

PID=$(ps aux|awk '$11 ~ "./vdm"{print $2}')


pstree -lup $PID

ps H -C vdm -o pid,ppid,tid,cmd,comm,wchan

ps H -C decoder -o pid,ppid,tid,cmd,comm,wchan

ps H -C audioplayer -o pid,ppid,tid,cmd,comm,wchan
