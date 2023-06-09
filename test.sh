#!/bin/zsh

# Build debug (./bin/release/parse)
# make -B 

# Build release (./bin/release/parse)
# make -B BUILD=release

# Test debug build
ok=0

time_cmd() {
    # /usr/bin/time --format="time: %E mem-peak: %M waits: %w" $*
    format='\n\treal: %e s\n\tmem-peak: %M kB'

    /usr/bin/time --format="$format" $*
}

for file in sample-files/*.json; do
    stderr_output=$(mktemp)

    echo -n "(debug) testing with $file: "

    time_cmd ./bin/debug/parse $file 1>/dev/null 2>$stderr_output

    result=$?

    if [ $result -ne 0 ]; then
        echo -n "failed ($result) "
        ok=1
    else
        echo -n "OK"
    fi

    if echo $output | grep -q "AddressSanitizer" ; then
        echo -n "AddressSanitizer triggered on $file"
        ok=1
    fi

    cat $stderr_output
    echo ""
done


# Test release build

for file in sample-files/*.json; do
    stderr_output=$(mktemp)

    echo -n "(release) testing with $file: "

    time_cmd ./bin/release/parse $file 1>/dev/null 2>$stderr_output

    result=$?

    if [ $result -ne 0 ]; then
        echo -n "failed ($result) "
        ok=1
    else
        echo -n "OK"
    fi

    cat $stderr_output
    echo ""
done

# compare with python JSON.load()
stderr_output=$(mktemp)
echo -n "(release) comparing with python: "
time_cmd python3 <<EOS 1>/dev/null 2>$stderr_output
import json
print(json.load(open("sample-files/large-file.json")))
EOS

cat $stderr_output

exit $ok
