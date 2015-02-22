//------------------------------------------------------------------------------
//  GfxResourceContainer.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "GfxResourceContainer.h"
#include "Core/Core.h"

namespace Oryol {

using namespace _priv;

//------------------------------------------------------------------------------
GfxResourceContainer::GfxResourceContainer() :
renderer(nullptr),
displayMgr(nullptr) {
    // empty
}

//------------------------------------------------------------------------------
void
GfxResourceContainer::setup(const GfxSetup& setup, class renderer* rendr, class displayMgr* dspMgr) {
    o_assert(!this->isValid());
    o_assert(nullptr != rendr);
    o_assert(nullptr != dspMgr);
    
    this->renderer = rendr;
    this->displayMgr = dspMgr;
    
    this->meshFactory.Setup(this->renderer, &this->meshPool);
    this->meshPool.Setup(GfxResourceType::Mesh, setup.PoolSize(GfxResourceType::Mesh));
    this->shaderFactory.Setup();
    this->shaderPool.Setup(GfxResourceType::Shader, setup.PoolSize(GfxResourceType::Shader));
    this->programBundleFactory.Setup(this->renderer, &this->shaderPool, &this->shaderFactory);
    this->programBundlePool.Setup(GfxResourceType::ProgramBundle, setup.PoolSize(GfxResourceType::ProgramBundle));
    this->textureFactory.Setup(this->renderer, this->displayMgr, &this->texturePool);
    this->texturePool.Setup(GfxResourceType::Texture, setup.PoolSize(GfxResourceType::Texture));
    this->drawStateFactory.Setup(&this->meshPool, &this->programBundlePool);
    this->drawStatePool.Setup(GfxResourceType::DrawState, setup.PoolSize(GfxResourceType::DrawState));
    
    resourceContainerBase::setup(setup.ResourceLabelStackCapacity, setup.ResourceRegistryCapacity);
}

//------------------------------------------------------------------------------
void
GfxResourceContainer::discard() {
    o_assert_dbg(this->isValid());
    
    resourceContainerBase::discard();

    for (const auto& loader : this->meshLoaders) {
        loader->OnDetach();
    }
    this->meshLoaders.Clear();
    for (const auto& loader : this->textureLoaders) {
        loader->OnDetach();
    }
    this->textureLoaders.Clear();
    
    this->drawStatePool.Discard();
    this->drawStateFactory.Discard();
    this->texturePool.Discard();
    this->textureFactory.Discard();
    this->programBundlePool.Discard();
    this->programBundleFactory.Discard();
    this->shaderPool.Discard();
    this->shaderFactory.Discard();
    this->meshPool.Discard();
    this->meshFactory.Discard();
    this->renderer = nullptr;
    this->displayMgr = nullptr;
}

//------------------------------------------------------------------------------
template<> Id
GfxResourceContainer::Create(const MeshSetup& setup) {
    o_assert_dbg(this->isValid());
    o_assert_dbg(!setup.ShouldSetupFromFile());
    Id resId = this->registry.Lookup(setup.Locator);
    if (resId.IsValid()) {
        return resId;
    }
    else {
        resId = this->meshPool.AllocId(this->peekLabel());
        mesh& res = this->meshPool.Assign(resId, setup);
        this->meshFactory.SetupResource(res);
        this->registry.Add(setup.Locator, resId);
    }
    return resId;
}
    
//------------------------------------------------------------------------------
template<> Id
GfxResourceContainer::Create(const SetupAndStream<MeshSetup>& setupAndStream) {
    o_assert_dbg(this->isValid());
    const MeshSetup& setup = setupAndStream.Setup;
    const Ptr<Stream>& data = setupAndStream.Stream;
    o_assert_dbg(!setup.ShouldSetupFromFile());
    Id resId = this->registry.Lookup(setup.Locator);
    if (resId.IsValid()) {
        return resId;
    }
    else {
        resId = this->meshPool.AllocId(this->peekLabel());
        mesh& res = this->meshPool.Assign(resId, setup);
        this->meshFactory.SetupResource(res, data);
        this->registry.Add(setup.Locator, resId);
    }
    return resId;
}

//------------------------------------------------------------------------------
template<> Id
GfxResourceContainer::Create(const TextureSetup& setup) {
    o_assert_dbg(this->isValid());
    o_assert_dbg(!setup.ShouldSetupFromFile());
    Id resId = this->registry.Lookup(setup.Locator);
    if (resId.IsValid()) {
        return resId;
    }
    else {
        resId = this->texturePool.AllocId(this->peekLabel());
        texture& res = this->texturePool.Assign(resId, setup);
        this->textureFactory.SetupResource(res);
        this->registry.Add(setup.Locator, resId);
    }
    return resId;
}
    
//------------------------------------------------------------------------------
template<> Id
GfxResourceContainer::Create(const SetupAndStream<TextureSetup>& setupAndStream) {
    o_assert_dbg(this->isValid());
    const TextureSetup& setup = setupAndStream.Setup;
    const Ptr<Stream>& data = setupAndStream.Stream;
    o_assert_dbg(!setup.ShouldSetupFromFile());
    Id resId = this->registry.Lookup(setup.Locator);
    if (resId.IsValid()) {
        return resId;
    }
    else {
        resId = this->texturePool.AllocId(this->peekLabel());
        texture& res = this->texturePool.Assign(resId, setup);
        this->textureFactory.SetupResource(res, data);
        this->registry.Add(setup.Locator, resId);
    }
    return resId;
}

//------------------------------------------------------------------------------
template<> Id
GfxResourceContainer::Load(const TextureSetup& setup) {
    o_assert_dbg(this->isValid());
    o_assert_dbg(setup.ShouldSetupFromFile());
    Id resId = this->registry.Lookup(setup.Locator);
    if (resId.IsValid()) {
        return resId;
    }
    else {
        Log::Info("GfxResourceContainer::Load(): LOADING TEXTURE!!!\n");
        resId = this->texturePool.AllocId(this->peekLabel());
        this->registry.Add(setup.Locator, resId);
        /*
        this->texturePool.Prepare(resId, setup);
        this->ioQueue.Add(setup.Locator.Location, [this,resId](const Ptr<Stream>& stream) {
            // this is executed when the file has finished (down)-loading
         
            // find pending resource object, it could have been deleted
            // while the file was loading, we're only interested in the 
            // original setup object
            texture* tex = this->texturePool.Get(resId);
            if (tex) {
                o_assert(ResourceState::Pending == pendingTex->State)
                SetupAndData input(tex->Setup, stream);
                for (const auto& loader : this->textureLoaders) {
                    if (loader->Accepts(input)) {
                        SetupAndStream<TextureSetup> result = loader->Load(input);
                        this->texturePool.Assign(resId, result.setup);
                        this->textureFactory.SetupResource(*tex, result.stream);
                        break;
                    }
                }
            }
            else {
                // the resource was destroyed while it was loading
            }
        });
        */
        return resId;
    }
}
    
//------------------------------------------------------------------------------
template<> Id
GfxResourceContainer::Create(const ShaderSetup& setup) {
    o_assert_dbg(this->isValid());
    Id resId = this->registry.Lookup(setup.Locator);
    if (resId.IsValid()) {
        return resId;
    }
    else {
        resId = this->shaderPool.AllocId(this->peekLabel());
        shader& res = this->shaderPool.Assign(resId, setup);
        this->shaderFactory.SetupResource(res);
        this->registry.Add(setup.Locator, resId);
    }
    return resId;
}
    
//------------------------------------------------------------------------------
template<> Id
GfxResourceContainer::Create(const ProgramBundleSetup& setup) {
    o_assert_dbg(this->isValid());
    Id resId = this->registry.Lookup(setup.Locator);
    if (resId.IsValid()) {
        return resId;
    }
    else {
        resId = this->programBundlePool.AllocId(this->peekLabel());
        programBundle& res = this->programBundlePool.Assign(resId, setup);
        this->programBundleFactory.SetupResource(res);
        this->registry.Add(setup.Locator, resId);
    }
    return resId;
}
    
//------------------------------------------------------------------------------
template<> Id
GfxResourceContainer::Create(const DrawStateSetup& setup) {
    o_assert_dbg(this->isValid());
    Id resId = this->registry.Lookup(setup.Locator);
    if (resId.IsValid()) {
        return resId;
    }
    else {
        resId = this->drawStatePool.AllocId(this->peekLabel());
        drawState& res = this->drawStatePool.Assign(resId, setup);
        this->drawStateFactory.SetupResource(res);
        this->registry.Add(setup.Locator, resId);
    }
    return resId;
}
    
//------------------------------------------------------------------------------
void
GfxResourceContainer::Destroy(uint8 label) {
    o_assert_dbg(this->isValid());
    Array<Id> ids = this->registry.Remove(label);
    for (const Id& id : ids) {
        switch (id.Type()) {
            case GfxResourceType::Texture:
            {
                texture* tex = this->texturePool.Lookup(id);
                if (tex) {
                    this->textureFactory.DestroyResource(*tex);
                }
                this->texturePool.Unassign(id);
            }
            break;
                
            case GfxResourceType::Mesh:
            {
                mesh* msh = this->meshPool.Lookup(id);
                if (msh) {
                    this->meshFactory.DestroyResource(*msh);
                }
                this->meshPool.Unassign(id);
            }
            break;
                
            case GfxResourceType::Shader:
            {
                shader* shd = this->shaderPool.Lookup(id);
                if (shd) {
                    this->shaderFactory.DestroyResource(*shd);
                }
                this->shaderPool.Unassign(id);
            }
            break;
                
            case GfxResourceType::ProgramBundle:
            {
                programBundle* prog = this->programBundlePool.Lookup(id);
                if (prog) {
                    this->programBundleFactory.DestroyResource(*prog);
                }
                this->programBundlePool.Unassign(id);
            }
            break;
                
            case GfxResourceType::ConstantBlock:
                o_assert2(false, "FIXME!!!\n");
                break;
                
            case GfxResourceType::DrawState:
            {
                drawState* ds = this->drawStatePool.Lookup(id);
                if (ds) {
                    this->drawStateFactory.DestroyResource(*ds);
                }
                this->drawStatePool.Unassign(id);
            }
            break;
                
            default:
                o_assert(false);
                break;
        }
    }
}
    
//------------------------------------------------------------------------------
ResourceState::Code
GfxResourceContainer::QueryState(const Id& resId) const {
    o_assert_dbg(this->isValid());
    switch (resId.Type()) {
        case GfxResourceType::Texture:
            return this->texturePool.QueryState(resId);
        case GfxResourceType::Mesh:
            return this->meshPool.QueryState(resId);
        case GfxResourceType::Shader:
            return this->shaderPool.QueryState(resId);
        case GfxResourceType::ProgramBundle:
            return this->programBundlePool.QueryState(resId);
        case GfxResourceType::ConstantBlock:
            o_assert2(false, "FIXME!!!\n");
            break;
        case GfxResourceType::DrawState:
            return this->drawStatePool.QueryState(resId);
        default:
            o_assert(false);
            break;
    }
    return ResourceState::InvalidState;
}

} // namespace Oryol