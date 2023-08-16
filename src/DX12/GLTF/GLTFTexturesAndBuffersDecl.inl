class GLTFTexturesAndBuffers
{
    Device     *m_pDevice;
    UploadHeap *m_pUploadHeap;

    const json *m_pTextureNodes;

    std::vector<Texture> m_images;
    std::vector<D3D12_STATIC_SAMPLER_DESC> m_samplerDescs;
    std::unordered_map <int, int> m_textureId2SamplerId;
    // texture id to image id, that is being referenced.
    std::unordered_map<int, int> m_textureToImage;

    std::map<int, D3D12_GPU_VIRTUAL_ADDRESS> m_skeletonMatricesBuffer;
    std::vector<D3D12_CONSTANT_BUFFER_VIEW_DESC> m_InverseBindMatrices;

    StaticBufferPool *m_pStaticBufferPool;
    DynamicBufferRing *m_pDynamicBufferRing;

    // maps GLTF ids into views
    std::map<int, D3D12_VERTEX_BUFFER_VIEW> m_vertexBufferMap;
    std::map<int, D3D12_INDEX_BUFFER_VIEW> m_IndexBufferMap;

public:
    GLTFCommon *m_pGLTFCommon;

    D3D12_GPU_VIRTUAL_ADDRESS m_perFrameConstants;

    bool OnCreate(Device* pDevice, GLTFCommon *pGLTFCommon, UploadHeap* pUploadHeap, StaticBufferPool *pStaticBufferPool, DynamicBufferRing *pDynamicBufferRing);
    void LoadTextures(AsyncPool *pAsyncPool, ID3D12Heap* pTextureHeap, uint64_t workloadId);
    void LoadGeometry();
    void OnDestroy();

    void CreateIndexBuffer(int indexBufferId, uint32_t *pNumIndices, DXGI_FORMAT *pIndexType, D3D12_INDEX_BUFFER_VIEW *pIBV);
    void CreateGeometry(int indexBufferId, std::vector<int> &vertexBufferIds, Geometry *pGeometry);
    void CreateGeometry(const json &primitive, const std::vector<std::string > requiredAttributes, std::vector<std::string> &semanticNames, std::vector<D3D12_INPUT_ELEMENT_DESC> &layout, DefineList &defines, Geometry *pGeometry);

    void SetPerFrameConstants();
    void SetSkinningMatricesForSkeletons();

    Texture *GetTextureViewByID(int id);
    D3D12_STATIC_SAMPLER_DESC GetSamplerDescByTextureId(int id);
    D3D12_GPU_VIRTUAL_ADDRESS GetSkinningMatricesBuffer(int skinIndex);
    D3D12_GPU_VIRTUAL_ADDRESS GetPerFrameConstants() { return m_perFrameConstants; }
};
