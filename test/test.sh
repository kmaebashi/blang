#!/bin/sh
../blang test.b > ./actual/test.result
diff ./expected/test.result ./actual/test.result
