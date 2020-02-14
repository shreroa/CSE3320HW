// The MIT License (MIT)
//
// Copyright (c) 2017 Trevor Bakker
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Purpose: Demonstrate reading strings and numbers from the command line

int main( int argc, char * argv[] )
{

  if( argc < 2 ) 
  {
    printf("Error: You must provide two command line parameters for this program\n" );
    printf("       You can run the program with: program_name [string] [number]\n");
    exit(1);
  }
  
  // We are assuming the user gives us a string as the first parameter and a number as the
  // second parameter.  No real error checking will be done.

  // Allocate a string to hold the first parameter.  We could just use argv[1] as is, but
  // we will do it the longer way in case you need this functionality later.
  char * string_param = (char*)malloc( strlen( argv[1] ) ); 

  // Copy the commandline string to our local variable
  memcpy( string_param, argv[1], strlen( argv[1] ) );

  // Use the atoi() function to convert our parameter from an ascii string to an integer
  int integer_param = atoi( argv[2] );

  printf("String parameter: %s\n", string_param );
  printf("Integer parameter: %d\n", integer_param );

  // Free up our allocated memory
  free( string_param );

  return 0;
}
