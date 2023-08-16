bool GLTFTexturesAndBuffers::OnCreate(Device *pDevice, GLTFCommon *pGLTFCommon, UploadHeap* pUploadHeap, StaticBufferPool *pStaticBufferPool, DynamicBufferRing *pDynamicBufferRing)
{
    m_pDevice = pDevice;
    m_pGLTFCommon = pGLTFCommon;
    m_pUploadHeap = pUploadHeap;
    m_pStaticBufferPool = pStaticBufferPool;
    m_pDynamicBufferRing = pDynamicBufferRing;

    return true;
}

void GLTFTexturesAndBuffers::LoadTextures(AsyncPool *pAsyncPool, ID3D12Heap* pTextureHeap, uint64_t workloadId)
{
    // load textures 
    //
    if (m_pGLTFCommon->j3.find("images") != m_pGLTFCommon->j3.end())
    {
        
        m_pTextureNodes = &m_pGLTFCommon->j3["textures"];
        const json &images = m_pGLTFCommon->j3["images"];
        const json &materials = m_pGLTFCommon->j3["materials"];
        const int textureNodeCount = m_pTextureNodes->size();
        const std::string nodeName("source");
        for (int textureIndex = 0; textureIndex < textureNodeCount; textureIndex++) {
            m_textureToImage[textureIndex] =   (*m_pTextureNodes)[textureIndex][nodeName].get<int>();
        }
        m_images.resize(images.size());
        for (int imageIndex = 0; imageIndex < images.size(); imageIndex++)
        {
            Texture *pTex = &m_images[imageIndex];
            std::string filename = m_pGLTFCommon->m_path + images[imageIndex]["uri"].get<std::string>();

            ExecAsyncIfThereIsAPool(pAsyncPool, L"TextureLoader", [imageIndex, pTex, this, filename, pTextureHeap, workloadId ,&materials]()
            {
                bool useSRGB;
                float cutOff;
                GetSrgbAndCutOffOfImageGivenItsUse(imageIndex, materials, m_textureToImage, &useSRGB, &cutOff);

                bool result = pTex->InitFromFile(m_pDevice, m_pUploadHeap, pTextureHeap, filename.c_str(), workloadId, useSRGB, cutOff);
                assert(result != false);
            });
        }

        // For each GLTF texture, there could be a sampler defined.
        const std::string samplerNodeName("sampler");
        std::vector<int> samplerIndices;
        for (int textureIndex = 0; textureIndex < textureNodeCount; textureIndex++)
        {
            if ((*m_pTextureNodes)[textureIndex].contains(samplerNodeName))
            {
                m_textureId2SamplerId[textureIndex] = (*m_pTextureNodes)[textureIndex][samplerNodeName].get<int>();
            }
            else
            {
                // Default to sampler id -1.
                m_textureId2SamplerId[textureIndex] = -1;
            }
        }

        // Create sampler descs.
        const json& samplers = m_pGLTFCommon->j3["samplers"];
        auto samplerCount = samplers.size();
        m_samplerDescs.resize(samplerCount);
        
        for (int samplerIdx = 0; samplerIdx < samplerCount; samplerIdx++)
        {
            int magFilter = 9729; // LINEAR
            if (samplers[samplerIdx].contains("magFilter"))
            {
                magFilter = samplers[samplerIdx]["magFilter"].get<int>();
            }

            int minFilter = 9729; // LINEAR
            if (samplers[samplerIdx].contains("minFilter"))
            {
                minFilter = samplers[samplerIdx]["minFilter"].get<int>();
            }

            int wrapS = 10497; // REPEAT
            if (samplers[samplerIdx].contains("wrapS"))
            {
                wrapS = samplers[samplerIdx]["wrapS"].get<int>();
            }

            int wrapT = 10497; // REPEAT
            if (samplers[samplerIdx].contains("wrapT"))
            {
                wrapT = samplers[samplerIdx]["wrapT"].get<int>();
            }

            auto GetSamplerDesc = [](int magFilter, int minFilter, int wrapS, int wrapT)
            {
                int d3d12FilterType = 0;
                switch (magFilter)
                {
                case 9728: // NEAREST
                    d3d12FilterType |= D3D12_ENCODE_BASIC_FILTER(0, D3D12_FILTER_TYPE_POINT, 0, 0);
                    break;
                case 9729: // LINEAR
                    d3d12FilterType |= D3D12_ENCODE_BASIC_FILTER(0, D3D12_FILTER_TYPE_LINEAR, 0, 0);
                    break;
                default:
                    throw std::invalid_argument("Unhandled mag filter type.");
                }

                switch (minFilter)
                {
                case 9728: // NEAREST
                    d3d12FilterType |= D3D12_ENCODE_BASIC_FILTER(D3D12_FILTER_TYPE_POINT, 0, 0, 0);
                    break;
                case 9729: // LINEAR
                    d3d12FilterType |= D3D12_ENCODE_BASIC_FILTER(D3D12_FILTER_TYPE_LINEAR, 0, 0, 0);
                    break;
                case 9984: // NEAREST_MIPMAP_NEAREST
                    d3d12FilterType |= D3D12_ENCODE_BASIC_FILTER(D3D12_FILTER_TYPE_POINT, 0, D3D12_FILTER_TYPE_POINT, 0);
                    break;
                case 9985: // LINEAR_MIPMAP_NEAREST
                    d3d12FilterType |= D3D12_ENCODE_BASIC_FILTER(D3D12_FILTER_TYPE_LINEAR, 0, D3D12_FILTER_TYPE_POINT, 0);
                    break;
                case 9986: // NEAREST_MIPMAP_LINEAR
                    d3d12FilterType |= D3D12_ENCODE_BASIC_FILTER(D3D12_FILTER_TYPE_POINT, 0, D3D12_FILTER_TYPE_LINEAR, 0);
                    break;
                case 9987: // LINEAR_MIPMAP_LINEAR
                    d3d12FilterType |= D3D12_ENCODE_BASIC_FILTER(D3D12_FILTER_TYPE_LINEAR, 0, D3D12_FILTER_TYPE_LINEAR, 0);
                    break;
                default:
                    throw std::invalid_argument("Unhandled min filter type.");
                }

                int addressModeU = 0;
                switch (wrapS)
                {
                    case 33071: // CLAMP_TO_EDGE
                        addressModeU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
                        break;
                    case 33648: // MIRRORED_REPEAT
                        addressModeU = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
                        break;
                    case 10497: // REPEAT
                        addressModeU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
                        break;
                    default: 
                        throw std::invalid_argument("Unhandled address mode.");
                    
                }

                int addressModeV = 0;
                switch (wrapT)
                {
                case 33071: // CLAMP_TO_EDGE
                    addressModeV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
                    break;
                case 33648: // MIRRORED_REPEAT
                    addressModeV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
                    break;
                case 10497: // REPEAT
                    addressModeV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
                    break;
                default:
                    throw std::invalid_argument("Unhandled address mode.");

                }



                D3D12_STATIC_SAMPLER_DESC samplerDesc = {};

                samplerDesc.Filter = (D3D12_FILTER)d3d12FilterType;
                samplerDesc.AddressU = (D3D12_TEXTURE_ADDRESS_MODE)addressModeU;
                samplerDesc.AddressV = (D3D12_TEXTURE_ADDRESS_MODE)addressModeV;
                samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
                samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
                samplerDesc.MinLOD = 0.0f;
                samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
                samplerDesc.MipLODBias = 0;
                samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
                samplerDesc.MaxAnisotropy = 1;
                samplerDesc.ShaderRegister = 0; // need to fill this in.
                samplerDesc.RegisterSpace = 0; // need to fill this in.
                samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

                return samplerDesc;
            };

            m_samplerDescs[samplerIdx] = GetSamplerDesc(magFilter, minFilter, wrapS, wrapT);
        }


        LoadGeometry();


        if (pAsyncPool)
            pAsyncPool->Flush();

        // copy textures and apply barriers, then flush the GPU
        if( m_pUploadHeap )
            m_pUploadHeap->FlushAndFinish();
    }
}

void GLTFTexturesAndBuffers::LoadGeometry()
{
    if (m_pGLTFCommon->j3.find("meshes") != m_pGLTFCommon->j3.end())
    {
        for (const json &mesh : m_pGLTFCommon->j3["meshes"])
        {
            for (const json &primitive : mesh["primitives"])
            {
                //
                //  Load vertex buffers
                //
                for (const json &attributeId : primitive["attributes"])
                {
                    tfAccessor vertexBufferAcc;
                    m_pGLTFCommon->GetBufferDetails(attributeId, &vertexBufferAcc);

                    D3D12_VERTEX_BUFFER_VIEW vbv = {};
                    m_pStaticBufferPool->AllocVertexBuffer(vertexBufferAcc.m_count, vertexBufferAcc.m_stride, vertexBufferAcc.m_data, &vbv);

                    m_vertexBufferMap[attributeId] = vbv;
                }

                //
                //  Load index buffers
                //
                int indexAcc = primitive.value("indices", -1);
                if (indexAcc >= 0)
                {
                    tfAccessor indexBufferAcc;
                    m_pGLTFCommon->GetBufferDetails(indexAcc, &indexBufferAcc);

                    D3D12_INDEX_BUFFER_VIEW ibv = {};

                    // Some exporters use 1-byte indices, need to convert them to shorts
                    if (indexBufferAcc.m_stride == 1)
                    {
                        unsigned short *pIndices = (unsigned short *)malloc(indexBufferAcc.m_count * (2 * indexBufferAcc.m_stride));
                        for (int i = 0; i < indexBufferAcc.m_count; i++)
                            pIndices[i] = ((unsigned char *)indexBufferAcc.m_data)[i];
                        m_pStaticBufferPool->AllocIndexBuffer(indexBufferAcc.m_count, 2 * indexBufferAcc.m_stride, pIndices, &ibv);
                        free(pIndices);
                    }
                    else
                    {
                        m_pStaticBufferPool->AllocIndexBuffer(indexBufferAcc.m_count, indexBufferAcc.m_stride, indexBufferAcc.m_data, &ibv);
                    }

                    m_IndexBufferMap[indexAcc] = ibv;
                }
            }
        }
    }
}

void GLTFTexturesAndBuffers::OnDestroy()
{
    for (int i = 0; i < m_images.size(); i++)
    {
        m_images[i].OnDestroy();
    }
}


D3D12_STATIC_SAMPLER_DESC GLTFTexturesAndBuffers::GetSamplerDescByTextureId(int id)
{
    auto sampId = m_textureId2SamplerId[id];
    if (sampId == -1)
    {
        // return default.
        D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
        samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        samplerDesc.MinLOD = 0.0f;
        samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
        samplerDesc.MipLODBias = 0;
        samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        samplerDesc.MaxAnisotropy = 1;
        samplerDesc.ShaderRegister = 0;
        samplerDesc.RegisterSpace = 0;
        samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        return samplerDesc;
    }
    else
    {
        return m_samplerDescs[sampId];
    }
}


Texture *GLTFTexturesAndBuffers::GetTextureViewByID(int id)
{
    int tex = m_pTextureNodes->at(id)["source"];
    return &m_images[tex];
}

// Creates a Index Buffer from the accessor
//
//
void GLTFTexturesAndBuffers::CreateIndexBuffer(int indexBufferId, uint32_t *pNumIndices, DXGI_FORMAT *pIndexType, D3D12_INDEX_BUFFER_VIEW *pIBV)
{
    tfAccessor indexBuffer;
    m_pGLTFCommon->GetBufferDetails(indexBufferId, &indexBuffer);

    *pNumIndices = indexBuffer.m_count;
    *pIndexType = (indexBuffer.m_stride == 4) ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;

    *pIBV = m_IndexBufferMap[indexBufferId];
}

// Creates Vertex Buffers from accessors and sets them in the Primitive struct.
//
//
void GLTFTexturesAndBuffers::CreateGeometry(int indexBufferId, std::vector<int> &vertexBufferIds, Geometry *pGeometry)
{
    CreateIndexBuffer(indexBufferId, &pGeometry->m_NumIndices, &pGeometry->m_indexType, &pGeometry->m_IBV);

    // load the rest of the buffers onto the GPU
    pGeometry->m_VBV.resize(vertexBufferIds.size());
    for (int i = 0; i < vertexBufferIds.size(); i++)
    {
        pGeometry->m_VBV[i] = m_vertexBufferMap[vertexBufferIds[i]];
    }
}

// Creates buffers and the input assemby at the same time. It needs a list of attributes to use.
//
void GLTFTexturesAndBuffers::CreateGeometry(const json &primitive, const std::vector<std::string> requiredAttributes, std::vector<std::string> &semanticNames, std::vector<D3D12_INPUT_ELEMENT_DESC> &layout, DefineList &defines, Geometry *pGeometry)
{
    // Get Index buffer view
    //
    tfAccessor indexBuffer;
    int indexBufferId = primitive.value("indices", -1);
    CreateIndexBuffer(indexBufferId, &pGeometry->m_NumIndices, &pGeometry->m_indexType, &pGeometry->m_IBV);

    // Create vertex buffers and input layout
    //
    int cnt = 0;
    layout.resize(requiredAttributes.size());
    semanticNames.resize(requiredAttributes.size());
    pGeometry->m_VBV.resize(requiredAttributes.size());
    const json &attributes = primitive.at("attributes");

    std::string hasSubString{"HAS_"};
    for (const auto& attrName : requiredAttributes)
    {
        // get vertex buffer view
        // 
        const int attr = attributes.find(attrName).value();
        pGeometry->m_VBV[cnt] = m_vertexBufferMap[attr];

        // Set define so the shaders knows this stream is available
        //
        defines[hasSubString + attrName] = std::string("1");

        // split semantic name from index, DX doesnt like the trailing number
        uint32_t semanticIndex = 0;
        SplitGltfAttribute(attrName, &semanticNames[cnt], &semanticIndex);

        const json &inAccessor = m_pGLTFCommon->m_pAccessors->at(attr);

        // Create Input Layout
        //
        D3D12_INPUT_ELEMENT_DESC l = {};
        l.SemanticName = semanticNames[cnt].c_str(); // we need to set it in the pipeline function (because of multithreading)
        l.SemanticIndex = semanticIndex;
        l.Format = GetFormat(inAccessor["type"], inAccessor["componentType"]);
        l.InputSlot = (UINT)cnt;
        l.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        l.InstanceDataStepRate = 0;
        l.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
        layout[cnt] = l;

        cnt++;            
    }
}

void GLTFTexturesAndBuffers::SetPerFrameConstants()
{
    m_perFrameConstants = m_pDynamicBufferRing->AllocConstantBuffer(sizeof(per_frame), &m_pGLTFCommon->m_perFrameData);
}

void GLTFTexturesAndBuffers::SetSkinningMatricesForSkeletons()
{
    for (auto &t : m_pGLTFCommon->m_worldSpaceSkeletonMats)
    {
        std::vector<Matrix2> *matrices = &t.second;
        D3D12_GPU_VIRTUAL_ADDRESS perSkeleton = m_pDynamicBufferRing->AllocConstantBuffer((uint32_t)(matrices->size() * sizeof(Matrix2)), matrices->data());

        m_skeletonMatricesBuffer[t.first] = perSkeleton;
    }
}

D3D12_GPU_VIRTUAL_ADDRESS GLTFTexturesAndBuffers::GetSkinningMatricesBuffer(int skinIndex)
{
    auto it = m_skeletonMatricesBuffer.find(skinIndex);

    if (it == m_skeletonMatricesBuffer.end())
        return NULL;

    return it->second;
}



