# Conbreak
### Usage
Please ensure you have lldb-3.6.1 and its required dependencies installed before using ConBreak.  Please refer to the README at the root of this repo for more installation information.

    jason:~/conbreak$ lldb [path_to_executable]
    (lldb) command script import conbreak.py
    (lldb) cbr <breakpoint [filename]:[line_number]>
    (lldb) run [options]

### Example

    jason:~/conbreak$ lldb apache-21287/httpd
    (lldb) command script import conbreak.py
    The "cbr" python command has been installed and is ready for use.
    (lldb) cbr mod_mem_cache:362
    Breakpoint set at mod_mem_cache.c:362
    (lldb) run -k start -X
    ...


