#!/bin/sh
../blang convert.b > ./actual/convert.result
diff ./expected/convert.result ./actual/convert.result
