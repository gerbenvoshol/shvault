# shvault

CLI secure password vault using SQLcipher

# Build & Install

```
$ git clone https://github.com/gerbenvoshol/shvault.git
$ cd pwvlt
$ make
$ make install
```

# Command syntax :

```
Usage: ./shvault_static [options] [key] [value]
Options:
  -s             Show value for key
  -c             Execute commands located in value
  -a             Append/insert entries
  -r             Replace entries
  -e             Erase entries
  -v <vault>     Specify vault (database file)
  -p <password>  Provide a password for the database
  -q             Type vault entry using prompt
  -f             Find and replace for vault templates using the following format find:replace
  -l             List all entries
  -h             Show this help message

Examples:
  export SHVAULT_PASSWORD=secret       Set 'secret' as the password for the database
  ./shvault_static -a key "value to insert"    Insert 'value to insert' under 'key'
  ./shvault_static -l                          List all entries in the database
  ./shvault_static -e key                      Delete the entry with 'key'
  ./shvault_static -v myvault.db -p secret     Use 'myvault.db' as the database with 'secret' as the password
  
  You can also append a string or password to a vault from stdin
  printf "MyPassword" | ./shvault_static -a MySecretPassword

  Use in authentication script
  some_application -username=myuser -password=$(./shvault_static -s MySecretPassword)

  Run as command :
  Last but not least you can store short command scripts and execute
  the vaulted string content as command(s). This is practical if you need to put commands
  in scripts that have sensitive strings or passwords in plain text. This will hide those
  strings or commands from the script.
  echo date | ./shvault_static -a MySecretCommand
  ./shvault_static -c MySecretCommand
  
  Safely store login scripts with hard coded passwords.
  Content of phrase.sh
  #!/bin/bash
  echo "{{ 1 }} {{ 2 }}!"
  
  ./shvault_static -a MySecretPhrase < phrase.sh
  ./shvault_static -c MySecretPhrase -f 1:Hello -f 2:World
```
