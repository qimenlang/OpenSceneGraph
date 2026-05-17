<?php
//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

beginModule( 'Triton' );

   // Look for the optional global from the project.conf.
   global $TRITON_PATH;
   if (!$TRITON_PATH)
   {
      // First look for an environment var.
      $TRITON_PATH = getenv( "TRITON_PATH" );

      if (strlen($TRITON_PATH) == 0 || !file_exists($TRITON_PATH))
      {
         // Sometimes users get confused and use this var.
         $TRITON_PATH = getenv( "TRITON_PATH" );

         if (strlen($TRITON_PATH) == 0 || !file_exists($TRITON_PATH))
         {
            // Since neither environment var worked try for 
            // the default Triton SDK install location.
            $TRITON_PATH = getenv("ProgramFiles") . "/Triton SDK";

            // Last channce... try the x86 default install path.
            if (!file_exists($TRITON_PATH))
               $TRITON_PATH = getenv("ProgramFiles(x86)") . "/Triton SDK";
         }
      }

      // We need forward slashes for paths.
      $TRITON_PATH = str_replace( "\\", "/", $TRITON_PATH);

      // Remove trailing slashes.
      $TRITON_PATH = rtrim($TRITON_PATH, " /");
   }

   // If we still don't have the SDK path then let the user know.
   if (!file_exists($TRITON_PATH))
   {
      trigger_error( 
            "\n*******************************************************************".
            "\n".
            "\n  We were not able to find a valid path to the Triton Ocean SDK!".
            "\n".
            "\n  You must install the latest Triton Ocean SDK and set the path via a".
            "\n  \$TRITON_PATH variable in your buildFiles/project.conf file".
            "\n  or by setting the TRITON_PATH system environment variable".
            "\n  (may require a reboot).".
            "\n".
            "\n*******************************************************************".
            "\n", E_USER_ERROR );
   }

   // Source
   addEngineSrcDir( "environment/tritonOcean" );

   // Includes
   addIncludePath( $TRITON_PATH . "/Public Headers" );

   // Libs
   addProjectLibDir( $TRITON_PATH . "/lib/vc10/win32" );
   addProjectLibInput( "Triton-MT.lib", "Triton-MTD.lib" );
   addProjectIgnoreDefaultLib( "MSVCRT" );
   
   function recurse_copy($src,$dst) { 
		$dir = opendir($src); 
		@mkdir($dst); 
		while(false !== ( $file = readdir($dir)) ) { 
			if (( $file != '.' ) && ( $file != '..' )) { 
				if ( is_dir($src . '/' . $file) ) { 
					recurse_copy($src . '/' . $file,$dst . '/' . $file); 
				} 
				else { 
					copy($src . '/' . $file,$dst . '/' . $file); 
				} 
			} 
		} 
		closedir($dir); 
	}
	
	recurse_copy($TRITON_PATH . "/Resources", getcwd() . "/game/Resources");

endModule();

?>
