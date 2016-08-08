@echo off
SET /A SEED=%RANDOM% %% 2147483647
START cmd /C %CD%\tw_minetee_srv.exe "sv_gametype minetee;sv_name Local MineTee Party;sv_max_clients 4;sv_map mt_test;sv_register 0;sv_rcon_password abc;sv_map_generation_size medium;sv_map_generation_seed %SEED%;sv_high_bandwidth 1"
START cmd /C %CD%\tw_minetee.exe 'connect localhost:8303'
