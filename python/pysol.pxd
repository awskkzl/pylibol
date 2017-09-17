cimport numpy as np

cdef extern from "sol/c_api.h":
    void* sol_CreateDataIter(int batch_size, int buf_size)
    void sol_ReleaseDataIter(void** data_iter)
    int sol_LoadData(void* data_iter, const char* path, const char* format, int pass_num)
    void* sol_CreateModel(const char* name, int class_num)
    void* sol_RestoreModel(const char* model_path)
    int sol_SaveModel(void* model, const char* model_path)
	
    #added by jing
    void sol_Getw(void* model, int classifier_id,float* w)
    int sol_Getw_dime(void* model, int classifier_id)
    
    void sol_ReleaseModel(void** model)
    int sol_SetModelParameter(void* model, const char* param_name, const char* param_val)
    ctypedef void (*get_parameter_callback)(void* user_context, const char* param_name, const char* param_val)
    int sol_GetModelParameters(void* model, get_parameter_callback callback, void* user_context)
    #modified by Jing
    float sol_Train(void* model, void* data_iter,void* data_no, void* iter_no,void* err_no,void*time_no, void* update_no,int* table_size);
    float sol_Test(void* model, void* data_iter,const char* output_path, float* tpr_fig, float*fpr_fig, float* tpr_tab, float* fpr_tab, float* auc);
    ctypedef void (*sol_predict_callback)(void* user_context, double label, double predict, int cls_num, float* scores)
    int sol_Predict(void* model, void* data_iter, sol_predict_callback callback, void* user_context)
    float sol_model_sparsity(void* model)
    ctypedef void (*inspect_iterate_callback)(void* user_context, long long data_num, long long iter_num,
                                         long long update_num, double err_rate)
    void sol_InspectOnlineIteration(void* model, inspect_iterate_callback callback, void* user_context)
    int sol_loadArray(void* data_iter, char* X, char* Y, np.npy_intp* dims, np.npy_intp* strides, int pass_num)
    int sol_loadCsrMatrix(void* data_iter, char* indices, char* indptr, char* features, char* Y, int n_samples, int pass_num)
    int sol_analyze_data(const char* data_path, const char* data_type, const char* output_path)
    int sol_convert_data(const char* src_path, const char* src_type, const char* dst_path, const char* dst_type, bint binarize, float binarize_thresh)
    int sol_shuffle_data(const char* src_path, const char* src_type, const char* dst_path, const char* dst_type)
    int sol_split_data(const char* src_path, const char* src_type, int fold, const char* output_prefix, const char* dst_type, bint shuffle)
