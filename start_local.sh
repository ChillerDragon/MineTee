#!/bin/bash

SEED=$(($RANDOM % 2147483647))
x-terminal-emulator -e "$PWD/tw_minetee_srv 'sv_gametype minetee;sv_name Local MineTee Party;sv_max_clients 4;sv_map mt_test;sv_register 0;sv_rcon_password abc;sv_map_generation_size medium;sv_map_generation_seed $SEED;sv_high_bandwidth 1'"
x-terminal-emulator -e "$PWD/tw_minetee 'connect localhost:8303'"
