// Copyright (c) 2020 vesoft inc. All rights reserved.
//
// This source code is licensed under Apache 2.0 License.

#include "graph/executor/maintain/TagExecutor.h"

#include "graph/planner/plan/Maintain.h"
#include "graph/util/SchemaUtil.h"

namespace nebula {
namespace graph {

folly::Future<Status> CreateTagExecutor::execute() {
  SCOPED_TIMER(&execTime_);

  auto *ctNode = asNode<CreateTag>(node());
  auto spaceId = qctx()->rctx()->session()->space().id;
  return qctx()
      ->getMetaClient()
      ->createTagSchema(spaceId, ctNode->getName(), ctNode->getSchema(), ctNode->getIfNotExists())
      .via(runner())
      .thenValue([ctNode, spaceId](StatusOr<TagID> resp) {
        memory::MemoryCheckGuard guard;
        if (!resp.ok()) {
          LOG(WARNING) << "SpaceId: " << spaceId << ", Create tag `" << ctNode->getName()
                       << "' failed: " << resp.status();
          return resp.status();
        }
        return Status::OK();
      })
      .thenError(
          folly::tag_t<std::bad_alloc>{},
          [](const std::bad_alloc &) { return folly::makeFuture<Status>(memoryExceededStatus()); })
      .thenError(folly::tag_t<std::exception>{}, [](const std::exception &e) {
        return folly::makeFuture<Status>(std::runtime_error(e.what()));
      });
}

folly::Future<Status> DescTagExecutor::execute() {
  SCOPED_TIMER(&execTime_);

  auto *dtNode = asNode<DescTag>(node());
  auto spaceId = qctx()->rctx()->session()->space().id;
  return qctx()
      ->getMetaClient()
      ->getTagSchema(spaceId, dtNode->getName())
      .via(runner())
      .thenValue([this, dtNode, spaceId](StatusOr<meta::cpp2::Schema> resp) {
        memory::MemoryCheckGuard guard;
        if (!resp.ok()) {
          LOG(WARNING) << "SpaceId: " << spaceId << ", Desc tag `" << dtNode->getName()
                       << "' failed: " << resp.status();
          return resp.status();
        }
        auto ret = SchemaUtil::toDescSchema(resp.value());
        if (!ret.ok()) {
          LOG(WARNING) << "SpaceId: " << spaceId << ", Desc tag `" << dtNode->getName()
                       << "' failed: " << resp.status();
          return ret.status();
        }
        return finish(
            ResultBuilder().value(std::move(ret).value()).iter(Iterator::Kind::kDefault).build());
      })
      .thenError(
          folly::tag_t<std::bad_alloc>{},
          [](const std::bad_alloc &) { return folly::makeFuture<Status>(memoryExceededStatus()); })
      .thenError(folly::tag_t<std::exception>{}, [](const std::exception &e) {
        return folly::makeFuture<Status>(std::runtime_error(e.what()));
      });
}

folly::Future<Status> DropTagExecutor::execute() {
  SCOPED_TIMER(&execTime_);

  auto *dtNode = asNode<DropTag>(node());
  auto spaceId = qctx()->rctx()->session()->space().id;
  return qctx()
      ->getMetaClient()
      ->dropTagSchema(spaceId, dtNode->getName(), dtNode->getIfExists())
      .via(runner())
      .thenValue([dtNode, spaceId](StatusOr<bool> resp) {
        memory::MemoryCheckGuard guard;
        if (!resp.ok()) {
          LOG(WARNING) << "SpaceId: " << spaceId << ", Drop tag `" << dtNode->getName()
                       << "' failed: " << resp.status();
          return resp.status();
        }
        return Status::OK();
      })
      .thenError(
          folly::tag_t<std::bad_alloc>{},
          [](const std::bad_alloc &) { return folly::makeFuture<Status>(memoryExceededStatus()); })
      .thenError(folly::tag_t<std::exception>{}, [](const std::exception &e) {
        return folly::makeFuture<Status>(std::runtime_error(e.what()));
      });
}

folly::Future<Status> ShowTagsExecutor::execute() {
  SCOPED_TIMER(&execTime_);

  auto spaceId = qctx()->rctx()->session()->space().id;
  return qctx()
      ->getMetaClient()
      ->listTagSchemas(spaceId)
      .via(runner())
      .thenValue([this, spaceId](StatusOr<std::vector<meta::cpp2::TagItem>> resp) {
        memory::MemoryCheckGuard guard;
        if (!resp.ok()) {
          LOG(WARNING) << "SpaceId: " << spaceId << ", Show tags failed: " << resp.status();
          return resp.status();
        }
        auto tagItems = std::move(resp).value();

        DataSet dataSet;
        dataSet.colNames = {"Name"};
        std::set<std::string> orderTagNames;
        for (auto &tag : tagItems) {
          orderTagNames.emplace(tag.get_tag_name());
        }
        for (auto &name : orderTagNames) {
          Row row;
          row.values.emplace_back(name);
          dataSet.rows.emplace_back(std::move(row));
        }
        return finish(ResultBuilder()
                          .value(Value(std::move(dataSet)))
                          .iter(Iterator::Kind::kDefault)
                          .build());
      })
      .thenError(
          folly::tag_t<std::bad_alloc>{},
          [](const std::bad_alloc &) { return folly::makeFuture<Status>(memoryExceededStatus()); })
      .thenError(folly::tag_t<std::exception>{}, [](const std::exception &e) {
        return folly::makeFuture<Status>(std::runtime_error(e.what()));
      });
}

folly::Future<Status> ShowCreateTagExecutor::execute() {
  SCOPED_TIMER(&execTime_);

  auto *sctNode = asNode<ShowCreateTag>(node());
  auto spaceId = qctx()->rctx()->session()->space().id;
  return qctx()
      ->getMetaClient()
      ->getTagSchema(spaceId, sctNode->getName())
      .via(runner())
      .thenValue([this, sctNode, spaceId](StatusOr<meta::cpp2::Schema> resp) {
        memory::MemoryCheckGuard guard;
        if (!resp.ok()) {
          LOG(WARNING) << "SpaceId: " << spaceId << ", Show create tag `" << sctNode->getName()
                       << "' failed: " << resp.status();
          return resp.status();
        }
        auto ret = SchemaUtil::toShowCreateSchema(true, sctNode->getName(), resp.value());
        if (!ret.ok()) {
          LOG(WARNING) << "SpaceId: " << spaceId << ", Show create tag `" << sctNode->getName()
                       << "' failed: " << resp.status();
          return ret.status();
        }
        return finish(
            ResultBuilder().value(std::move(ret).value()).iter(Iterator::Kind::kDefault).build());
      })
      .thenError(
          folly::tag_t<std::bad_alloc>{},
          [](const std::bad_alloc &) { return folly::makeFuture<Status>(memoryExceededStatus()); })
      .thenError(folly::tag_t<std::exception>{}, [](const std::exception &e) {
        return folly::makeFuture<Status>(std::runtime_error(e.what()));
      });
}

folly::Future<Status> AlterTagExecutor::execute() {
  SCOPED_TIMER(&execTime_);

  auto *aeNode = asNode<AlterTag>(node());
  return qctx()
      ->getMetaClient()
      ->alterTagSchema(
          aeNode->space(), aeNode->getName(), aeNode->getSchemaItems(), aeNode->getSchemaProp())
      .via(runner())
      .thenValue([aeNode](StatusOr<bool> resp) {
        memory::MemoryCheckGuard guard;
        if (!resp.ok()) {
          LOG(WARNING) << "SpaceId: " << aeNode->space() << ", Alter tag `" << aeNode->getName()
                       << "' failed: " << resp.status();
          return resp.status();
        }
        return Status::OK();
      })
      .thenError(
          folly::tag_t<std::bad_alloc>{},
          [](const std::bad_alloc &) { return folly::makeFuture<Status>(memoryExceededStatus()); })
      .thenError(folly::tag_t<std::exception>{}, [](const std::exception &e) {
        return folly::makeFuture<Status>(std::runtime_error(e.what()));
      });
}
}  // namespace graph
}  // namespace nebula
