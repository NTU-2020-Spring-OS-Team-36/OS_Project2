TEST_PATH=$1
METHOD=$2
IP=$3

FILE_COUNT=0
TEST_FILES=""
OUTPUT_FILES=""

[ -d "output/${TEST_PATH}" ] && rm output/$TEST_PATH/ -r
mkdir -p output/$TEST_PATH

for FILE in $TEST_PATH/*; do
	TEST_FILES="${TEST_FILES} ${FILE}"
	OUTPUT_FILES="${OUTPUT_FILES} output/${FILE}"
	FILE_COUNT=$((FILE_COUNT + 1))
done

(./user_program/master $FILE_COUNT $TEST_FILES $METHOD) &
./user_program/slave $FILE_COUNT $OUTPUT_FIELS $METHOD $IP

