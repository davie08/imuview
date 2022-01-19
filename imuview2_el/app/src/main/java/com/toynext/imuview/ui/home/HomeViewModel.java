package com.toynext.imuview.ui.home;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.ViewModel;

import com.toynext.imuview.R;

public class HomeViewModel extends ViewModel {

    private MutableLiveData<String> mText;

    public HomeViewModel() {
        mText = new MutableLiveData<>();
        mText.setValue("init...");
    }

    public void SetString(String str) {
        if (mText != null) {
            mText.setValue(str);
        }
    }
    public LiveData<String> getText() {
        return mText;
    }
}