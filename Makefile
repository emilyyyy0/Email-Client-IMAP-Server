EXE=fetchmail

$(EXE): main.c imap_client.c utils.c server_response.c
	cc -Wall -o $(EXE) $^

# Rust
# $(EXE): src/*.rs vendor
# 	cargo build --frozen --offline --release
# 	cp target/release/$(EXE) .

# vendor:
# 	if [ ! -d "vendor/" ]; then \
# 		cargo vendor --locked; \
# 	fi

test: 
	./fetchmail -f Test -p pass -u test@comp30023 -n 1 retrieve unimelb-comp30023-2024.cloud.edu.au

nofolderexist: 
	./fetchmail -u test@comp30023 -p pass -n 1 -f Test1 retrieve unimelb-comp30023-2024.cloud.edu.au

loginfail: 
	./fetchmail -f Test -u test@comp30023 -p pass1 -n 1 retrieve unimelb-comp30023-2024.cloud.edu.au

nofolderspecified: 
	./fetchmail -n 1 -p pass -u test@comp30023 mime unimelb-comp30023-2024.cloud.edu.au 

withspace: 
	./fetchmail -f 'With Space' -n 1 -u test@comp30023 -p pass retrieve unimelb-comp30023-2024.cloud.edu.au

noMessageNum:
	./fetchmail -p pass -u test@comp30023 -f Test retrieve unimelb-comp30023-2024.cloud.edu.au

rawEmail: 
	./fetchmail -u test.test@comp30023 -p -p -f Test -n 1 retrieve unimelb-comp30023-2024.cloud.edu.au

numNotFound: 
	./fetchmail -n 42 -u test@comp30023 -p pass -f Test retrieve unimelb-comp30023-2024.cloud.edu.au

retrieve: 
	./fetchmail -f Test -p pass -u test@comp30023 -n 1 retrieve unimelb-comp30023-2024.cloud.edu.au
	./fetchmail -f Test -p pass -u test@comp30023 -n 2 retrieve unimelb-comp30023-2024.cloud.edu.au
	./fetchmail -u test.test@comp30023 -p -p -f Test -n 1 retrieve unimelb-comp30023-2024.cloud.edu.au
	./fetchmail -f 'With Space' -n 1 -u test@comp30023 -p pass retrieve unimelb-comp30023-2024.cloud.edu.au


mime: 
	./fetchmail -n 1 -p pass -u test@comp30023 mime unimelb-comp30023-2024.cloud.edu.au 
	./fetchmail -f Test -n 2 -p pass -u test@comp30023 mime unimelb-comp30023-2024.cloud.edu.au 

mimediff: 
	./fetchmail -n 1 -p pass -u test@comp30023 mime unimelb-comp30023-2024.cloud.edu.au | diff - out/mime-ed512.out
	./fetchmail -f Test -n 2 -p pass -u test@comp30023 mime unimelb-comp30023-2024.cloud.edu.au | diff - out/mime-mst.out







run: 
	./fetchmail -f Test -p pass -n 2 -u test@comp30023 parse unimelb-comp30023-2024.cloud.edu.au 
	./fetchmail -f Test -n 3 -p pass -u test@comp30023 parse unimelb-comp30023-2024.cloud.edu.au 
	./fetchmail -p pass -f headers -u test@comp30023 -n 2 parse unimelb-comp30023-2024.cloud.edu.au 
	./fetchmail -f headers -u test@comp30023 -p pass -n 3 parse unimelb-comp30023-2024.cloud.edu.au
	./fetchmail -u test@comp30023 -n 4 -p pass -f headers parse unimelb-comp30023-2024.cloud.edu.au
	./fetchmail -f headers -u test@comp30023 -n 5 -p pass parse unimelb-comp30023-2024.cloud.edu.au

	./fetchmail -n 1 -p pass -u test@comp30023 mime unimelb-comp30023-2024.cloud.edu.au 
	./fetchmail -f Test -n 2 -p pass -u test@comp30023 mime unimelb-comp30023-2024.cloud.edu.au 

	./fetchmail -p pass -u test@comp30023 -f Test list unimelb-comp30023-2024.cloud.edu.au
	./fetchmail -p pass -u test@comp30023 list unimelb-comp30023-2024.cloud.edu.au 

	./fetchmail -f Test -p pass -u test@comp30023 -n 1 -t retrieve unimelb-comp30023-2024.cloud.edu.au

format:
	clang-format -style=file -i *.c

clean: 
	rm -f fetchmail



# ./fetchmail -f Test -p pass -u test@comp30023 -n 2 retrieve unimelb-comp30023-2024.cloud.edu.au
# ./fetchmail -f Test -u test@comp30023 -p pass1 -n 1 retrieve unimelb-comp30023-2024.cloud.edu.au
# ./fetchmail -u test@comp30023 -p pass -n 1 -f Test1 retrieve unimelb-comp30023-2024.cloud.edu.au
# ./fetchmail -n 42 -u test@comp30023 -p pass -f Test retrieve unimelb-comp30023-2024.cloud.edu.au
# ./fetchmail -u test.test@comp30023 -p -p -f Test -n 1 retrieve unimelb-comp30023-2024.cloud.edu.au
# ./fetchmail -f 'With Space' -n 1 -u test@comp30023 -p pass retrieve unimelb-comp30023-2024.cloud.edu.au

# ./fetchmail -f Test -p pass -n 2 -u test@comp30023 parse unimelb-comp30023-2024.cloud.edu.au 
# ./fetchmail -f Test -n 3 -p pass -u test@comp30023 parse unimelb-comp30023-2024.cloud.edu.au 
# ./fetchmail -p pass -f headers -u test@comp30023 -n 2 parse unimelb-comp30023-2024.cloud.edu.au 
# ./fetchmail -f headers -u test@comp30023 -p pass -n 3 parse unimelb-comp30023-2024.cloud.edu.au
# ./fetchmail -u test@comp30023 -n 4 -p pass -f headers parse unimelb-comp30023-2024.cloud.edu.au
# ./fetchmail -f headers -u test@comp30023 -n 5 -p pass parse unimelb-comp30023-2024.cloud.edu.au

# ./fetchmail -n 1 -p pass -u test@comp30023 mime unimelb-comp30023-2024.cloud.edu.au 
# ./fetchmail -f Test -n 2 -p pass -u test@comp30023 mime unimelb-comp30023-2024.cloud.edu.au 

# ./fetchmail -p pass -u test@comp30023 -f Test list unimelb-comp30023-2024.cloud.edu.au
# ./fetchmail -p pass -u test@comp30023 list unimelb-comp30023-2024.cloud.edu.au 

# ./fetchmail -f Test -p pass -u test@comp30023 -n 1 -t retrieve unimelb-comp30023-2024.cloud.edu.au




# ./fetchmail -f Test -p pass -u test@comp30023 -n 1 retrieve unimelb-comp30023-2024.cloud.edu.au | diff - out/ret-ed512.out
# ./fetchmail -f Test -p pass -u test@comp30023 -n 2 retrieve unimelb-comp30023-2024.cloud.edu.au | diff - out/ret-mst.out
# ./fetchmail -f Test -u test@comp30023 -p pass1 -n 1 retrieve unimelb-comp30023-2024.cloud.edu.au | diff - out/ret-loginfail.out
# ./fetchmail -u test@comp30023 -p pass -n 1 -f Test1 retrieve unimelb-comp30023-2024.cloud.edu.au | diff - out/ret-nofolder.out
# ./fetchmail -n 42 -u test@comp30023 -p pass -f Test retrieve unimelb-comp30023-2024.cloud.edu.au | diff - out/ret-nomessage.out
# ./fetchmail -u test.test@comp30023 -p -p -f Test -n 1 retrieve unimelb-comp30023-2024.cloud.edu.au | diff - out/ret-mst.out
# ./fetchmail -f 'With Space' -n 1 -u test@comp30023 -p pass retrieve unimelb-comp30023-2024.cloud.edu.au | diff - out/ret-mst.out

# ./fetchmail -f Test -p pass -n 2 -u test@comp30023 parse unimelb-comp30023-2024.cloud.edu.au | diff - out/parse-mst.out
# ./fetchmail -f Test -n 3 -p pass -u test@comp30023 parse unimelb-comp30023-2024.cloud.edu.au | diff - out/parse-minimal.out
# ./fetchmail -p pass -f headers -u test@comp30023 -n 2 parse unimelb-comp30023-2024.cloud.edu.au | diff - out/parse-caps.out
# ./fetchmail -f headers -u test@comp30023 -p pass -n 3 parse unimelb-comp30023-2024.cloud.edu.au | diff - out/parse-nosubj.out
# ./fetchmail -u test@comp30023 -n 4 -p pass -f headers parse unimelb-comp30023-2024.cloud.edu.au | diff - out/parse-nested.out
# ./fetchmail -f headers -u test@comp30023 -n 5 -p pass parse unimelb-comp30023-2024.cloud.edu.au | diff - out/parse-ws.out

# ./fetchmail -n 1 -p pass -u test@comp30023 mime unimelb-comp30023-2024.cloud.edu.au | diff - out/mime-ed512.out
# ./fetchmail -f Test -n 2 -p pass -u test@comp30023 mime unimelb-comp30023-2024.cloud.edu.au | diff - out/mime-mst.out

# ./fetchmail -p pass -u test@comp30023 -f Test list unimelb-comp30023-2024.cloud.edu.au | diff - out/list-Test.out
# ./fetchmail -p pass -u test@comp30023 list unimelb-comp30023-2024.cloud.edu.au | diff - out/list-INBOX.out

# ./fetchmail -f Test -p pass -u test@comp30023 -n 1 -t retrieve unimelb-comp30023-2024.cloud.edu.au | diff - out/ret-ed512.out
