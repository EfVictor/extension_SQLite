# extension_SQLite
 Examples of creating extensions for SQLite   
All examples are compiled in 32-bit format for the Windows operating system.

---------------

### Project structure:
																
	 length_function - implementation of creating a custom function to calculate the length of a string
	 loader_txt - implementation of a virtual table for reading data from a text file in SQLite

The `sources` folder stores the necessary header files for compilation  
The `sqlite` folder stores 32-bit sqlite files for testing compiled extensions  
`.c` files are the main files with all the logic of the user extensions described above


---------------

### Useful SQLite commands:													
															

  * Checking if SQLite build works with external extensions:    
```
PRAGMA compile_options;
```
If there is one on the list __ENABLE_LOAD_EXTENSION__ then the assembly supports loading extensions

  * Connecting extensions to SQLite:  
```
PRAGMA enable_load_extension = TRUE;
PRAGMA load_extension = 'path_to_extension';
```

  * List of available extensions. After connecting, the downloaded extension should appear:    
```
PRAGMA module_list;
```

---------------

### Examples code:

***IT IS IMPORTANT THAT THE BIT DEGREE OF THE EXTENSION (DLL) AND THE BIT DEGREE OF SQLITE + SQLITE3.DLL ARE THE SAME.***

1. Extension for calculating the length of a string:

command for compilation:  
```
gcc -shared -o length_function.dll length_function.c -IC:/extension_SQLite/length_function -LC:/extension_SQLite/length_function -lsqlite3
```  

SQLITE COMMANDS:  
```
.load "./length_function.dll" 
SELECT length_function('Hello World');
```

2. Extension for reading data from a .txt file into a virtual table:

command for compilation:  
```
gcc -shared -o loaderTXT.dll loaderTXT.c -IC:/extension_SQLite/txt -LC:/extension_SQLite/txt -lsqlite3
```

SQLITE COMMANDS:
```
.load "./loaderTXT.dll"
CREATE VIRTUAL TABLE table_TXT USING loaderTXT('test.txt'); 
SELECT * FROM table_TXT;
```

