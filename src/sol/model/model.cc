/*********************************************************************************
*     File Name           :     model.cc
*     Created By          :     yuewu
*     Creation Date       :     [2016-02-16 22:54]
*     Last Modified       :     [2016-12-04 18:04]
*     Description         :     base class for model
**********************************************************************************/

#include "sol/model/model.h"

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <string>

#include "sol/util/util.h"
#include "sol/util/error_code.h"

using namespace std;
using namespace sol::math::expr;
using namespace sol::pario;

namespace sol {
namespace model {

// comparison function
bool comp(pair<int, double> a, pair<int, double> b)
{
	return(a.second > b.second);
}

Model* Model::Create(const std::string& name, int class_num) {
  auto create_func = CreateObject<Model>(std::string(name) + "_model");
  Model* ins = nullptr;
  try {
    if (create_func != nullptr) ins = create_func(class_num);
  }
  catch (invalid_argument& err) {
    cerr << "create model failed: " << err.what() << "\n";
    ins = nullptr;
  }
  return ins;
}

Model::Model(int class_num, const std::string& type)
    : class_num_(class_num),
      clf_num_(class_num == 2 ? 1 : class_num),
      loss_(nullptr),
      type_(type),
      norm_type_(op::OpType::kNone),
      regularizer_(nullptr),
      max_index_(0) {
  Check(class_num > 1);
  this->update_num_ = 0;
  this->require_reinit_ = true;
  this->model_updated_ = false;
}

Model::~Model() { DeletePointer(this->loss_); }

void Model::SetParameter(const std::string& name, const std::string& value) {

  if (name == "model") {
    Check(value == this->name());
  } else if (name == "cls_num") {
    Check(stoi(value) == this->class_num());
  } else if (name == "clf_num") {
    Check(stoi(value) == this->clf_num());
  } else if (name == "loss") {
    DeletePointer(this->loss_);
    this->loss_ = loss::Loss::Create(value);
    Check(this->loss_ != nullptr);
    if (this->clf_num_ == 1 &&
        ((this->loss_->type() & loss::Loss::Type::BC) == 0)) {
      throw invalid_argument(
          "onlye binary loss function can be used with binary problems");
    } else if (this->clf_num_ != 1 &&
               ((this->loss_->type() & loss::Loss::Type::MC) == 0)) {
      throw invalid_argument(
          "onlye multiclass loss function can be used with multiclass "
          "problems");
    }
    this->require_reinit_ = true;
  } else if (name == "norm") {
    int norm_val = -1;
    try {
      string::size_type sz;
      norm_val = stoi(value, &sz);
      if (value[sz] != '\0') {
        norm_val = -1;
      }
    }
    catch (invalid_argument&) {
      norm_val = -1;
    }
    if (value == "None" || norm_val == op::OpType::kNone) {
      this->norm_type_ = op::OpType::kNone;
    } else if (value == "L1" || norm_val == op::OpType::kL1) {
      this->norm_type_ = op::OpType::kL1;
    } else if (value == "L2" || norm_val == op::OpType::kL2) {
      this->norm_type_ = op::OpType::kL2;
    } else {
      ostringstream oss;
      oss << "unknown norm type " << value;
      throw invalid_argument(oss.str());
    }
  } else if (name == "filter") {
    if (this->LoadPreSelFeatures(value) != Status_OK) {
      ostringstream oss;
      oss << "load pre-selected features failed!";
      throw invalid_argument(oss.str());
    }
  } else {
    if (this->regularizer_ == nullptr ||
        this->regularizer_->SetParameter(name, value) != Status_OK) {
      ostringstream oss;
      oss << "unknown parameter " << name;
      throw invalid_argument(oss.str());
    }
  }
}

float Model::Test(DataIter& data_iter, std::ostream* os,float* tpr_fig, float*fpr_fig, float* tpr_tab, float* fpr_tab,float* auc_out) {
  if (this->model_updated_) this->EndTrain();

  size_t err_num = 0;
  size_t data_num = 0;

  if (os != nullptr) {
    (*os) << "label\tpredict\tscores\n";
  }

  vector<pair<int, double>> prediction;//by Jing


  float* predicts = new float[this->clf_num()];
  MiniBatch* mb = nullptr;
  while (1) {
    mb = data_iter.Next(mb);
    if (mb == nullptr) break;
    // data_num += mb->size();
    for (int i = 0; i < mb->size(); ++i) {
      DataPoint& x = (*mb)[i];
      this->PreProcess(x);
      // predict
      label_t label = this->Predict(x, predicts);

	  //by Jing for auc calculation, only for binary classification
	  if (tpr_fig != NULL)
	  {
		  prediction.push_back(pair<int, double>(x.label(), predicts[0]));
	  }


      if (label != x.label()) err_num++;
      if (os != nullptr) {
        (*os) << x.label() << "\t" << label;
        for (int k = 0; k < this->clf_num_; ++k) {
          (*os) << "\t" << predicts[k];
        }
        (*os) << "\n";
      }
      ++data_num;
    }
  }
  delete[] predicts;


  //by Jing

  if (tpr_fig != NULL)
  {
	  int positive = 0;
	  int negative = 0;

	  for (int i = 0; i < prediction.size(); i++)
	  {
		  if (prediction[i].first > 0)
			  positive++;
		  else
			  negative++;
	  }

	  sort(prediction.begin(), prediction.end(), comp);

	  int TP = 0;
	  int FP = 0;


	  long long AUC = 0;

	  float target_TP = 1;
	  float target_FPR = 1e-5;

	  int index = 0;
	  int index2 = 0;
	  for (int i = 0; i < prediction.size(); i++)
	  {
		  if (prediction[i].first > 0)
		  {
			  TP++;
			  //calculate the AUC
			  AUC = AUC + FP;
		  }
		  else
		  {
			  FP++;
		  }

		  float frequence = 0.05;
		  if ((TP > target_TP) || (i == prediction.size() - 1))
		  {
			  target_TP = target_TP + frequence*positive;

			  tpr_fig[index] = (float)TP / positive;
			  fpr_fig[index]= (float)FP / negative;
			  index++;
		  }

		  if ((FP > target_FPR*negative) || (i == prediction.size() - 1))
		  {
			  fpr_tab[index2]=target_FPR;
			  tpr_tab[index2]=(float)TP / positive;
			  target_FPR = target_FPR * 10;
			  index2++;
		  }
	  }

	  *auc_out = 1 - ((float)AUC / positive) / negative;
  }




  return float(double(err_num) / data_num);
}

void Model::BeginTrain() {
  if (this->loss_ == nullptr)
    throw runtime_error("loss function is not set yet!");

  this->require_reinit_ = false;
}

int Model::Save(const string& path) const {
  ofstream out_file(path.c_str(), ios::out);
  if (!out_file) {
    cerr << "open file " << path << " failed\n";
    return Status_IO_Error;
  }
  Json::Value root;
  this->GetModelInfo(root);
  Json::StyledStreamWriter writer;
  out_file << "model info:\n";
  writer.write(out_file, root);

  out_file << "model parameters:\n";
  this->GetModelParam(out_file);

  out_file.close();
  return Status_OK;
}
//added by jing
math::Vector<real_t>* Model::Get() const
{
	return GetModelWeight();
}

Model* Model::Load(const string& path) {
  int ret = Status_OK;
  Model* model = nullptr;
  ifstream in_file(path.c_str(), ios::in);
  if (!in_file) {
    cerr << "open file " << path << " failed\n";
    return nullptr;
  }
  string line;
  getline(in_file, line);
  if (line != "model info:") {
    cerr << "invalid model file\n";
    ret = Status_Invalid_Format;
  }
  if (ret == Status_OK) {
    ostringstream model_info;
    while (line != "model parameters:") {
      getline(in_file, line);
      model_info << line;
    }
    Json::Value root;
    Json::Reader reader;
    if (reader.parse(model_info.str(), root) == false) {
      cerr << "parse model file " << path << " failed\n";
      ret = Status_Invalid_Format;
    }

    if (ret == Status_OK) {
      string cls_name = root.get("model", "").asString();
      int cls_num = root.get("cls_num", "0").asInt();
      model = Model::Create(cls_name, cls_num);
      if (model == nullptr) {
        cerr << "create model failed: no model named " << cls_name << "\n";
        ret = Status_Invalid_Format;
      } else {
        try {
          ret = model->SetModelInfo(root);
        }
        catch (invalid_argument& err) {
          cerr << "set model parameter failed: " << err.what() << "\n";
          ret = Status_Invalid_Argument;
        }
      }
    }
  }

  if (ret == Status_OK) {
    ret = model->SetModelParam(in_file);
  }
  if (ret != Status_OK) {
    DeletePointer(model);
    DeletePointer(model);
  }
  in_file.close();
  return model;
}

void Model::GetModelInfo(Json::Value& root) const {
  root["model"] = this->name();
  root["cls_num"] = this->class_num();
  root["clf_num"] = this->clf_num();
  root["loss"] = this->loss_ == nullptr ? "" : this->loss_->name();
  root["norm"] = int(this->norm_type_);

  // regularizer
  if (this->regularizer_ != nullptr) {
    this->regularizer_->GetRegularizerInfo(root);
  }
}

int Model::SetModelInfo(const Json::Value& root) {
  try {
    Check(root.get("model", "").asString() == this->name());
    Check(root.get("cls_num", "").asInt() == this->class_num());
    Check(root.get("clf_num", "").asInt() == this->clf_num());
  }
  catch (invalid_argument& err) {
    cerr << "set model info failed: " << err.what() << "\n";
    return Status_Invalid_Argument;
  }
  // loss
  if (this->loss_ == nullptr ||
      root["loss"].asString() != this->loss_->name()) {
    this->SetParameter("loss", root["loss"].asString());
  }
  // norm
  this->SetParameter("norm", root["norm"].asString());

  // regularizer
  const Json::Value& relu_settings = root["regularizer"];
  if (!(relu_settings.isNull()) && this->regularizer_ != nullptr) {
    for (Json::Value::const_iterator iter = relu_settings.begin();
         iter != relu_settings.end(); ++iter) {
      if (this->regularizer_->SetParameter(iter.name(), iter->asString()) !=
          Status_OK) {
        cerr << "unrecognized regularizer option: " << iter.name() << "\n";
        return Status_Invalid_Argument;
      }
    }
  }

  return Status_OK;
}

string Model::model_info() const {
  Json::Value root;
  this->GetModelInfo(root);

  Json::StyledWriter writer;
  return writer.write(root);
}

void Model::model_info(Json::Value& info) const { this->GetModelInfo(info); }

void Model::PreProcess(DataPoint& x) {
  // calibrate label
  x.set_label(this->CalibrateLabel(x.label()));
  // filter features
  this->FilterFeatures(x);

  // normalize
  if (this->norm_type_ != op::OpType::kNone) {
    real_t norm = 1;
    switch (this->norm_type_) {
      case op::OpType::kL1:
        norm = reduce<op::plus>(L1(x.data()));
        x.data() /= norm;
        break;
      case op::OpType::kL2:
        norm = reduce<op::plus>(L2(x.data()));
        x.data() /= sqrt(norm);
      default:
        break;
    }
  }
}

void Model::FilterFeatures(DataPoint& x) {
  if (this->max_index_ == 0) return;
  size_t feat_num = x.size();
  for (size_t i = 0; i < feat_num; ++i) {
    if (x.index(i) > this->max_index_ ||
        this->sel_feat_flags_[x.index(i)] == 0) {
      x.feature(i) = 0;
    }
  }
}

int Model::LoadPreSelFeatures(const string& path) {
  this->max_index_ = 0;
  this->sel_feat_flags_.clear();

  ifstream in_file(path.c_str(), ios::in);
  if (!in_file) {
    cerr << "open file " << path << " failed\n!";
    return Status_IO_Error;
  }

  index_t index = 0;
  string line;
  vector<index_t> indexes;
  // load feature indexes
  while (getline(in_file, line)) {
    const char* p = line.c_str();
    while (*p == ' ' || *p == '\t') ++p;
    // skip comments
    if (*p == '#') continue;

    index = (index_t)(stoi(line));
    if (index <= 0) {
      cerr << "parse index " << line << " failed!\n";
      return Status_Invalid_Format;
    }
    indexes.push_back(index);
  }

  // find the max index
  this->max_index_ = *(std::max_element)(indexes.begin(), indexes.end());
  this->sel_feat_flags_.reserve(this->max_index_ + 1);
  this->sel_feat_flags_.resize(this->max_index_ + 1);
  this->sel_feat_flags_ = 0;

  for (index_t i : indexes) {
    this->sel_feat_flags_[i] = 1;
  }

  return Status_OK;
}

}  // namespace model
}  // namespace sol
