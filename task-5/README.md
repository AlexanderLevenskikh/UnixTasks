# Task manager (daemon)

Logs: `/var/log/syslog`
Restart `kill -SIGHUP ${parent_pid}`

If command file is corrupted or not exists (unkonown command) - 60 min delay before retry

For testing:
```
bash run.sh
```
