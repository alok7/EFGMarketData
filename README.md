## Before Run
update folder path in 
- Config/marketdata_info.cfg
    * read_shm_queue_cfg
    * write_shm_queue_cfg
    * sessions_cfg
- Config/sessions.cfg
    * subscription_file
- Config/include/MarketData/compile_config.h
    * MD_LOG_DIR (optional, output directory for log files)


## Command to Build and Run
```bash
mkdir build
cd build
cmake ..
cmake --build .
./MarketData-bin ../Config/marketdata_info.cfg
```
## Note
Need to have access to SHM
