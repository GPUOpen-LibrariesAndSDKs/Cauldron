mkdir DX12
cd DX12
cmake -A x64 ..\.. -DGFX_API=DX12
cd ..

mkdir VK
cd VK
cmake -A x64 ..\.. -DGFX_API=VK
cd ..