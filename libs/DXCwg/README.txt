Note for IHVs:
      - D3D12 runtime is same as in AgilitySDK package, extracted here for convenience.
      - dxcompiler.dll is built from the sm68-core branch, 
        SHA 64ae9ea2147a51466cfae454bb53caeb14dcf81a
      - The binaries here are x64 binaries. Other architectures 
        can be set up for on request, feel free to contact the Direct3D team.

Note to Microsoft folks preparing drops:

Put SDK dependencies here, for example:
	- the Direct3D 12 Agility SDK, e.g., D3D12Core.dll
	- the shader compiler, e.g., dxcompiler.dll and dxc.exe
	
The test applications under /extras:
	- Set D3D12SDKPath pointed to this folder.
	- If they compile shaders, they expect the shader compiler to live in this folder.
	
