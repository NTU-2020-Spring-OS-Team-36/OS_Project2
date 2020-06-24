TEST_PATH="input/"
OUTPUT_PATH="output/"

for dir in $TEST_PATH/*; do
	for method in f m; do
		printf "=== %s (%s) ===\n" "$dir" "$method"
		out_path="${OUTPUT_PATH}/$(basename "$dir")_${method}"
		mkdir -p "$out_path"
		in_list="$(find "$dir" -type f | sort)"
		out_list="$(find "$dir" -type f -exec basename {} \; | sed -e "s#^#${out_path}/#" | sort)"
		eval ./user_program/master -$method $in_list &
		eval ./user_program/slave -$method 127.0.0.1 $out_list | tee result.txt
		wait
		diff -r "$dir" "$out_path"
		mv result.txt "$out_path"
	done
done
