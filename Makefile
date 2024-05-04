EXE=fetchmail

$(EXE): main.c imap_client.c utils.c
	cc -Wall -o $(EXE) $^

# Rust
# $(EXE): src/*.rs vendor
# 	cargo build --frozen --offline --release
# 	cp target/release/$(EXE) .

# vendor:
# 	if [ ! -d "vendor/" ]; then \
# 		cargo vendor --locked; \
# 	fi

run: 
	./fetchmail -f Test -p pass -u test@comp30023 -n 1 retrieve unimelb-comp30023-2024.cloud.edu.au

format:
	clang-format -style=file -i *.c

clean: 
	rm -f fetchmail
