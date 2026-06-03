// Copyright 2026 The OpenGrm Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "opengrm/thrax/walker/util/function/features.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "gtest/gtest.h"
#include "absl/base/casts.h"
#include "absl/container/btree_set.h"
#include "absl/log/check.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/symbol-table.h"
#include "openfst/lib/topsort.h"
#include "openfst/lib/vector-fst.h"
#include "opengrm/thrax/walker/util/datatype.h"
#include "opengrm/thrax/walker/util/function/stringfst.h"

namespace thrax {
namespace function {

class FeatureTest : public ::testing::Test {
 protected:
  using Arc = ::fst::StdArc;
  using Transducer = ::fst::Fst<Arc>;
  using MutableTransducer = ::fst::VectorFst<Arc>;

  void TearDown() override { StringFst<Arc>::ClearSymbolLabelMapForTest(); }

  virtual void RunTest(const std::vector<std::string>& feature_and_values) {
    auto args = std::make_unique<std::vector<std::unique_ptr<DataType>>>();
    for (size_t i = 0; i < feature_and_values.size(); ++i) {
      args->push_back(std::make_unique<DataType>(feature_and_values[i]));
    }
    std::unique_ptr<DataType> result(func_.Run(std::move(args)));
    auto* fst = absl::down_cast<MutableTransducer*>(
        *result->template get<Transducer*>());
    // Check this is a valid fst
    std::string feature;
    CHECK(Feature<Arc>::ValidateFeatureFst(fst, &feature));
    ASSERT_EQ(feature, feature_and_values[0]);
    // Now examine all of the labels in detail and compare them to what we
    // should be getting.
    std::vector<int64_t> labels;
    GatherLabels(*fst, &labels);
    ASSERT_EQ(feature_and_values.size() - 1, labels.size());
    std::vector<std::string> golden_featvals;
    for (size_t i = 1; i < feature_and_values.size(); ++i) {
      golden_featvals.push_back(feature_and_values[0] + kFeatureEquals +
                                feature_and_values[i]);
    }
    std::unique_ptr<const ::fst::SymbolTable> generated_symbols(
        StringFst<Arc>::GetLabelSymbolTable(false));
    std::vector<std::string> generated_featvals;
    for (size_t i = 0; i < labels.size(); ++i) {
      generated_featvals.push_back(generated_symbols->Find(labels[i]));
    }
    ASSERT_EQ(golden_featvals.size(), generated_featvals.size());
    std::sort(golden_featvals.begin(), golden_featvals.end());
    std::sort(generated_featvals.begin(), generated_featvals.end());
    for (size_t i = 0; i < generated_featvals.size(); ++i) {
      ASSERT_EQ(golden_featvals[i], generated_featvals[i]);
    }
  }

  virtual void GatherLabels(const MutableTransducer& fst,
                            std::vector<int64_t>* labels) {
    labels->clear();
    CHECK_EQ(fst.NumStates(), 2);
    ::fst::ArcIterator<Transducer> aiter(fst, fst.Start());
    while (!aiter.Done()) {
      const auto& arc = aiter.Value();
      labels->push_back(arc.olabel);
      aiter.Next();
    }
  }

  Feature<Arc> func_;
};

TEST_F(FeatureTest, BasicTest) {
  std::vector<std::string> case_feature;
  case_feature.push_back("case");
  case_feature.push_back("nom");
  case_feature.push_back("gen");
  case_feature.push_back("acc");
  case_feature.push_back("dat");
  case_feature.push_back("loc");
  RunTest(case_feature);
}

class CategoryTest : public ::testing::Test {
 protected:
  using Arc = ::fst::StdArc;
  using Transducer = ::fst::Fst<Arc>;
  using MutableTransducer = ::fst::VectorFst<Arc>;

  void TearDown() override { StringFst<Arc>::ClearSymbolLabelMapForTest(); }

  // Auxiliary function to make a Feature acceptor given a vector of a feature
  // and values
  virtual std::unique_ptr<MutableTransducer> MakeFeature(
      const std::vector<std::string>& feature_and_values) {
    auto args = std::make_unique<std::vector<std::unique_ptr<DataType>>>();
    for (size_t i = 0; i < feature_and_values.size(); ++i) {
      args->push_back(std::make_unique<DataType>(feature_and_values[i]));
    }
    std::unique_ptr<DataType> result(feature_func_.Run(std::move(args)));
    auto result_fst = std::make_unique<MutableTransducer>(
        **result->template get<Transducer*>());
    return result_fst;
  }

  virtual void RunTest(std::vector<std::unique_ptr<MutableTransducer>> features,
                       const std::vector<std::string>& golden_feature_names) {
    auto args = std::make_unique<std::vector<std::unique_ptr<DataType>>>();
    for (auto& feature : features) {
      args->push_back(std::make_unique<DataType>(std::move(feature)));
    }
    std::unique_ptr<DataType> result(category_func_.Run(std::move(args)));
    auto* fst =
        absl::down_cast<MutableTransducer*>(*result->get<Transducer*>());
    CHECK(fst->NumStates() == features.size() + 1);
    // Check that the result is valid and that the feature names all correspond
    std::vector<std::pair<Arc::StateId, std::string>> feature_names;
    CHECK(Category<Arc>::ValidateFeatureSequenceFst(fst, &feature_names));
    ASSERT_EQ(golden_feature_names.size(), feature_names.size());
    std::vector<std::string> sorted_golden_feature_names(golden_feature_names);
    std::sort(sorted_golden_feature_names.begin(),
              sorted_golden_feature_names.end());
    for (size_t i = 0; i < sorted_golden_feature_names.size(); ++i) {
      ASSERT_EQ(sorted_golden_feature_names[i], feature_names[i].second);
    }
  }

  Feature<Arc> feature_func_;
  Category<Arc> category_func_;
};

TEST_F(CategoryTest, BasicTest) {
  std::vector<std::string> feature;
  std::vector<std::unique_ptr<MutableTransducer>> features;
  std::vector<std::string> feature_names;
  feature.push_back("number");
  feature_names.push_back("number");
  feature.push_back("sg");
  feature.push_back("pl");
  features.push_back(MakeFeature(feature));
  feature.clear();
  feature.push_back("gender");
  feature_names.push_back("gender");
  feature.push_back("masc");
  feature.push_back("fem");
  feature.push_back("neut");
  features.push_back(MakeFeature(feature));
  feature.clear();
  feature.push_back("case");
  feature_names.push_back("case");
  feature.push_back("nom");
  feature.push_back("gen");
  feature.push_back("acc");
  feature.push_back("dat");
  feature.push_back("loc");
  features.push_back(MakeFeature(feature));
  feature.clear();
  RunTest(std::move(features), feature_names);
}

class FeatureVectorTest : public ::testing::Test {
 protected:
  using Arc = ::fst::StdArc;
  using Transducer = ::fst::Fst<Arc>;
  using MutableTransducer = ::fst::VectorFst<Arc>;

  void TearDown() override { StringFst<Arc>::ClearSymbolLabelMapForTest(); }

  // Auxiliary function to make a Feature acceptor given a vector of a feature
  // and values.
  virtual std::unique_ptr<MutableTransducer> MakeFeature(
      const std::vector<std::string>& features_and_values) {
    auto args = std::make_unique<std::vector<std::unique_ptr<DataType>>>();
    for (const auto& feature_and_value : features_and_values) {
      args->push_back(std::make_unique<DataType>(feature_and_value));
    }
    std::unique_ptr<DataType> result(feature_func_.Run(std::move(args)));
    auto result_fst = std::make_unique<MutableTransducer>(
        **result->template get<Transducer*>());
    return result_fst;
  }

  // Auxiliary function to make a Category acceptor given a vector of a Feature
  // acceptors.
  virtual std::unique_ptr<MutableTransducer> MakeCategory(
      std::vector<std::unique_ptr<MutableTransducer>> features) {
    auto args = std::make_unique<std::vector<std::unique_ptr<DataType>>>();
    for (auto& feature : features) {
      args->push_back(std::make_unique<DataType>(std::move(feature)));
    }
    std::unique_ptr<DataType> result(category_func_.Run(std::move(args)));
    auto category_fst = std::make_unique<MutableTransducer>(
        **result->template get<Transducer*>());
    return category_fst;
  }

  virtual void RunTest(std::unique_ptr<MutableTransducer> category,
                       const std::vector<std::string>& featvals) {
    const int category_num_states = category->NumStates();
    std::vector<std::pair<Arc::StateId, std::string>> feature_names;
    Category<Arc>::ValidateFeatureSequenceFst(category.get(), &feature_names);
    std::vector<int> category_num_arcs;
    for (Arc::StateId s = 0; s < category_num_states; ++s) {
      category_num_arcs.push_back(category->NumArcs(s));
    }
    auto args = std::make_unique<std::vector<std::unique_ptr<DataType>>>();
    args->push_back(std::make_unique<DataType>(std::move(category)));
    for (const auto& featval : featvals) {
      args->push_back(std::make_unique<DataType>(featval));
    }
    std::unique_ptr<DataType> result(feature_vector_func_.Run(std::move(args)));
    auto* fst = absl::down_cast<MutableTransducer*>(
        *result->template get<Transducer*>());
    CHECK_EQ(category_num_states, fst->NumStates());
    absl::btree_set<std::string> specified_features;
    for (const auto& featval : featvals) {
      std::string feature;
      SplitFeatureValue(featval, &feature);
      specified_features.insert(feature);
    }
    ::fst::TopSort(fst);
    for (const auto& pair : feature_names) {
      const auto feat_s = pair.first;
      const auto& feature = pair.second;
      const auto it = specified_features.find(feature);
      if (it == specified_features.end()) {
        CHECK_EQ(fst->NumArcs(feat_s), category_num_arcs[feat_s]);
      } else {
        CHECK_EQ(fst->NumArcs(feat_s), 1);
      }
    }
  }

  Feature<Arc> feature_func_;
  Category<Arc> category_func_;
  FeatureVector<Arc> feature_vector_func_;
};

TEST_F(FeatureVectorTest, BasicTest) {
  std::vector<std::vector<std::string>> feats;
  std::vector<std::string> feature;
  std::vector<std::unique_ptr<MutableTransducer>> features;
  feature.push_back("number");
  feature.push_back("sg");
  feature.push_back("pl");
  features.push_back(MakeFeature(feature));
  feature.clear();
  feature.push_back("gender");
  feature.push_back("masc");
  feature.push_back("fem");
  feature.push_back("neut");
  features.push_back(MakeFeature(feature));
  feature.clear();
  feature.push_back("case");
  feature.push_back("nom");
  feature.push_back("gen");
  feature.push_back("acc");
  feature.push_back("dat");
  feature.push_back("loc");
  features.push_back(MakeFeature(feature));
  std::unique_ptr<MutableTransducer> cat = MakeCategory(std::move(features));
  std::vector<std::string> featvals;
  featvals.push_back("case=acc");
  featvals.push_back("number=pl");
  RunTest(std::move(cat), featvals);
}

}  // namespace function
}  // namespace thrax
