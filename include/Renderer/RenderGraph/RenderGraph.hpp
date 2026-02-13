#pragma once

#include "ImageResource.hpp"
#include "RenderPass.hpp"

#include <list>
#include <memory>
// #include <ranges>
#include <string>
#include <vector>
#include <utility>
#include <variant>
#include <unordered_set>

namespace RenderGraph {
    enum class AddResult : uint8_t {
        OK = 0,
        ALREADY_PRESENT,
        ALREADY_COMPILED
    };
    enum class CompileResult : uint8_t {
        OK = 0,
        UNAVAILABLE_RESOURCE,
        ALREADY_COMPILED
    };
    enum class GetResourceError : uint8_t {
        DOES_NOT_EXISTS
    };
    enum class BindResourceResult : uint8_t {
        OK = 0,
        DOES_NOT_EXISTS
    };

    class RenderGraph {
        private:
            std::unordered_map<std::string, ImageResource> m_resources;

        public:
            using UnsortedNodes = std::list<std::unique_ptr<RenderPass>>;
            using OrderedNodes = std::vector<std::unique_ptr<RenderPass>>;

        private:
            std::variant<UnsortedNodes, OrderedNodes> m_nodes;

        public:
            RenderGraph() : m_nodes(UnsortedNodes{}) {}

        public:
            template<typename... Args>
            AddResult AddResource(Args&&... args) {
                if (std::holds_alternative<OrderedNodes>(m_nodes)) {
                    return AddResult::ALREADY_COMPILED;
                }  

                ImageResource temp(std::forward<Args>(args)...);
                if (m_resources.contains(temp.name)) {
                    return AddResult::ALREADY_PRESENT;
                }

                m_resources.insert({ temp.name, std::move(temp) });

                return AddResult::OK;
            }

            template<typename T, typename... Args>
            requires isRenderPass<T>
            AddResult AddRenderPass(Args&&... args) {
                if (std::holds_alternative<OrderedNodes>(m_nodes)) {
                    return AddResult::ALREADY_COMPILED;
                }
                UnsortedNodes& nodes = std::get<UnsortedNodes>(m_nodes);

                std::unique_ptr<RenderPass> temp = std::make_unique<T>(std::forward<Args>(args)...);
                if (std::ranges::find_if(nodes, 
                            [&temp] (std::unique_ptr<RenderPass>& pass) {
                                return *pass == *temp;
                            }) != nodes.end()) {
                    return AddResult::ALREADY_PRESENT;
                }

                nodes.emplace_back(std::move(temp));

                return AddResult::OK;
            }

        public:
            BindResourceResult BindExternalResource(const std::string& name, vk::Image image, vk::ImageView imageView) {
                auto _resource = getResource(name);
                if (!_resource) return BindResourceResult::DOES_NOT_EXISTS;
            
                ImageResource& res = _resource.value().get();
                res.image = image;
                res.view = imageView;
                res.initialLayout = vk::ImageLayout::eUndefined;
            
                return BindResourceResult::OK;
            }

        public:
            using GetResourceReturnType = std::expected<std::reference_wrapper<ImageResource>, GetResourceError>; 
            GetResourceReturnType getResource(const std::string& name) {
                auto resourceItr = m_resources.find(name);
                if (resourceItr == m_resources.end()) {
                    return std::unexpected(GetResourceError::DOES_NOT_EXISTS);
                }

                ImageResource& resource = resourceItr->second;
                return resource;
            }

            using GetConstResourceReturnType = std::expected<std::reference_wrapper<const ImageResource>, GetResourceError>; 
            GetConstResourceReturnType getResource(const std::string& name) const {
                const auto resourceItr = m_resources.find(name);
                if (resourceItr == m_resources.end()) {
                    return std::unexpected(GetResourceError::DOES_NOT_EXISTS);
                }

                const ImageResource& resource = resourceItr->second;
                return resource;
            }

            ImageResource& getResourceUnsafe(const std::string& name) {
                return getResource(name).value().get();
            }

        public:
            CompileResult Compile() {
                if (std::holds_alternative<OrderedNodes>(m_nodes)) {
                    return CompileResult::ALREADY_COMPILED;
                }

                UnsortedNodes& pendingNodes = std::get<UnsortedNodes>(m_nodes);
                OrderedNodes finalNodes{};
                finalNodes.reserve(pendingNodes.size());

                std::unordered_set<std::string> availableResources;
                auto canExecute = [&availableResources] (const RenderPass* renderPass) -> bool {
                        return renderPass->reads.empty() || std::ranges::all_of(renderPass->reads,
                            [&availableResources] (const std::string& read) -> bool {
                                return availableResources.contains(read);
                            });
                    };

                
                std::vector<std::list<std::unique_ptr<RenderPass>>::iterator> toErase;
                toErase.reserve(pendingNodes.size());
                while (!pendingNodes.empty()) {
                    toErase.clear();

                    for (auto itr = pendingNodes.begin(); itr != pendingNodes.end(); ++itr) {
                        if (canExecute(itr->get())) {
                            toErase.emplace_back(itr);

                            for (const std::string& input : (*itr)->reads) {
                                (*itr)->readImages.insert({ input, &m_resources.at(input) });
                            }
                            for (const std::string& output : (*itr)->writes) {
                                (*itr)->writeImages.insert({ output, &m_resources.at(output) });
                                availableResources.insert(output);
                            }

                            finalNodes.emplace_back(std::move(*itr));
                        }
                    }

                    if (toErase.empty()) break;

                    for (const auto& itr : toErase) {
                        pendingNodes.erase(itr);
                    }
                }

                if (!pendingNodes.empty()) {
                    return CompileResult::UNAVAILABLE_RESOURCE;
                }

                m_nodes.emplace<OrderedNodes>(std::move(finalNodes)); // this also destructs UnsortedNodes

                return CompileResult::OK;
            }

        public:
            enum class GetNodesError {
                NOT_COMPILED,
                NO_NODES
            };
            std::expected<std::reference_wrapper<const OrderedNodes>, GetNodesError> getOrderedNodes() const {
                if (std::holds_alternative<UnsortedNodes>(m_nodes)) {
                    return std::unexpected(RenderGraph::GetNodesError::NOT_COMPILED);
                }
                const OrderedNodes& nodes = std::get<OrderedNodes>(m_nodes);

                if (nodes.empty()) {
                    return std::unexpected(RenderGraph::GetNodesError::NO_NODES);
                }

                return nodes;
            }

            OrderedNodes& getOrderedNodesUnsafe() {
                return const_cast<OrderedNodes&>(getOrderedNodes().value().get());
            }
    };
}
