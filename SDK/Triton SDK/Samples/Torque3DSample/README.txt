Using Triton Ocean SDK with Torque 3D in Visual Studio 2010

1. Copy the contents of our provided Engine and Tools directories into your Torque 3D installation.

2. Make sure you have the Torque Project Manager installed, as we will need it to generate your project.

3. If you have a Triton license code, enter that in the initTriton() method of 
   Engine/source/environment/tritonOcean/tritonOcean.cpp.

4. Edit the "projects.xml" file for the project manager.

Open the file and find:

<entry type="modules">
Once you find that, add the following line afterwards:

<module name="Triton">Triton Ocean SDK</module>

5. Launch the project manager, and create a new project, or edit the modules of a current project. While creating
   the project, click "choose modules" and ensure the Triton Ocean SDK is selected. Allow the project manager to
   create your project files.

6. You need to modify a script in your project's world editor, so find and open: 
   My Projects\<Your Project>\game\tools\worldEditor\scripts\editors\creator.ed.cs

In this file find

%this.registerMissionObject( "WaterPlane",          "Water Plane" );

below it add

%this.registerMissionObject( "TritonOcean",         "Triton Ocean" );
%this.registerMissionObject( "TritonLocalWind",     "Triton Local Wind" );

This will allow you to add the Triton Ocean object to your scene. 

7. Build your project using the project file created by the project manager, under
   My Projects\<your project name>\buildFiles\VisualStudio 2010

8. Run your project, play it and hit F11 to get into the world editor. You should see a "Triton Ocean" in your library 
   under "Level / Environment".

9. Adjust the Z position of Triton's transform as needed to adjust the sea level.
