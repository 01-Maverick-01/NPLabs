# Network Programming - Lab1

This repo has my implementation for Lab1 where given a binary/text file and a pattern. As part of this lab, we had to display the file size and number of occurrence of given pattern in the input file. 

### Environment: [ OS: Ubuntu 18.04.3 LTS; Compiler: gcc (7.4.0); GNU Make (4.1) ]

### Execution instruction:
**cmdLine: count &lt;input-filename&gt; &lt;search-string&gt; &lt;output-filename&gt;** <br />
*input-filename*: relative/absolute path to input file <br />
*search-string*: pattern to search <br />
*output-filename*: relative/absolute path to output file <br />

### Implementation Logic:
* Validate cmdLine args. If failed, terminate execution.
* Open output file handle in binary write mode. If failed, display error and terminate execution.
* Open input file handle in binary read mode. If failed, display error on console and terminate execution. Also write error message in output file.
* Process file and calculate file size and number of occurrence of given pattern.
    * Read file in an increment of 80 bytes using 'fread' library function.
    * For every read operation:
        * Increment value of fileSize by value returned by 'fread'.
        * Get the number of of occurrences of pattern in current buffer.
            * Use 'memchr' to get position of 'pattern[0]' in current buffer.
            * If found, use 'memcmp' to check if pattern is present or not.
                * If 'memcmp' returned TRUE, increment count and repeat above to steps to find any other occurrence.
            * return count. 
        * Increment the value of count by number of occurrences. <br />
        **NOTE: In order to account for case when pattern is split between two reads, we concatenate last 'strlen(pattern) - 1' bytes of previous read opperation to current read buffer before searching for pattern.**
    * Display result on console. Save output in specified output file.
