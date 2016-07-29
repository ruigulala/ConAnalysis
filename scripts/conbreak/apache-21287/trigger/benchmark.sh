#!/bin/bash

ab -n 1000 -c 100 127.0.1.1:7000/pippo.php?variable=88
