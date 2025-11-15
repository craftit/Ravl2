#pragma once

#include <memory>
#include <vector>

#include "Ravl2/Display/ISceneNode.hh"

namespace Ravl2::DebugDisplay {

//! Composite scene node that renders multiple children in order.
//! Used to compose base images with overlays in a unified scene graph.
class CompositeNode : public ISceneNode {
public:
  CompositeNode() = default;
  ~CompositeNode() override = default;

  // Disable copy, allow move
  CompositeNode(const CompositeNode&) = delete;
  CompositeNode& operator=(const CompositeNode&) = delete;
  CompositeNode(CompositeNode&&) = default;
  CompositeNode& operator=(CompositeNode&&) = default;

  //! Add a child node to the end of the render list
  void addChild(std::unique_ptr<ISceneNode> node)
  {
    m_children.push_back(std::move(node));
  }

  //! Insert a child at a specific index
  void insertChild(size_t index, std::unique_ptr<ISceneNode> node)
  {
    if (index >= m_children.size()) {
      m_children.push_back(std::move(node));
    } else {
      m_children.insert(m_children.begin() + static_cast<std::vector<std::unique_ptr<ISceneNode>>::difference_type>(index), std::move(node));
    }
  }

  //! Replace child at index (returns old child)
  [[nodiscard]] std::unique_ptr<ISceneNode> replaceChild(size_t index, std::unique_ptr<ISceneNode> node)
  {
    if (index >= m_children.size()) {
      return nullptr;
    }
    std::unique_ptr<ISceneNode> old = std::move(m_children[index]);
    m_children[index] = std::move(node);
    return old;
  }

  //! Remove all children
  void clear()
  {
    m_children.clear();
  }

  //! Get number of children
  [[nodiscard]] size_t childCount() const
  {
    return m_children.size();
  }

  //! Access children (const)
  [[nodiscard]] const std::vector<std::unique_ptr<ISceneNode>>& children() const
  {
    return m_children;
  }

  //! Access children (mutable) - use with care
  [[nodiscard]] std::vector<std::unique_ptr<ISceneNode>>& children()
  {
    return m_children;
  }

  //! ISceneNode interface: prepare all children
  void prepare(RenderContext& ctx) override
  {
    for (auto& child : m_children) {
      if (child) {
        child->prepare(ctx);
      }
    }
  }

  //! ISceneNode interface: render all children in order
  void render(RenderContext& ctx) override
  {
    for (auto& child : m_children) {
      if (child) {
        child->render(ctx);
      }
    }
  }

  //! Query support: forwarded to first child that supports it (typically base image)
  bool supportsPixelQuery() const override
  {
    for (const auto& child : m_children) {
      if (child && child->supportsPixelQuery()) {
        return true;
      }
    }
    return false;
  }

  //! Pixel query: forwarded to first supporting child
  PixelQueryResult queryPixelInfo(int x, int y) const override
  {
    for (const auto& child : m_children) {
      if (child && child->supportsPixelQuery()) {
        return child->queryPixelInfo(x, y);
      }
    }
    return {.valid = false, .coordinateText = "", .valueText = "", .extraInfo = std::nullopt};
  }

private:
  std::vector<std::unique_ptr<ISceneNode>> m_children;
};

} // namespace Ravl2::DebugDisplay
