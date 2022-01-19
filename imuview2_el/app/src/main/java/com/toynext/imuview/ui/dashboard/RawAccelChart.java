package com.toynext.imuview.ui.dashboard;

import android.content.Context;
import android.graphics.Color;
import android.os.Bundle;
import androidx.fragment.app.Fragment;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import com.github.mikephil.charting.charts.LineChart;
import com.github.mikephil.charting.components.Legend;
import com.github.mikephil.charting.components.XAxis;
import com.github.mikephil.charting.components.YAxis;
import com.github.mikephil.charting.data.Entry;
import com.github.mikephil.charting.data.LineData;
import com.github.mikephil.charting.data.LineDataSet;
import com.github.mikephil.charting.interfaces.datasets.ILineDataSet;

import java.util.ArrayList;

public class RawAccelChart {

    private LineChart mChartView;

    private LineDataSet CreateLineDataSet(int mcolor, String mLabel)
    {
        LineDataSet set = new LineDataSet(null, "Data");
        set.setColor(mcolor);
        set.setLineWidth(3f);
        set.setHighlightEnabled(false);
        set.setLabel(mLabel);
        set.setAxisDependency(YAxis.AxisDependency.LEFT);
        set.setDrawValues(false);
        set.setDrawCircles(false);
        set.setMode(LineDataSet.Mode.CUBIC_BEZIER);
        set.setCubicIntensity(0.2f);
        return set;
    }

    public void AddAccelEntry(float valueX, float valueY, float valueZ)
    {
        LineData data = mChartView.getData();
        if (data != null)
        {
            ILineDataSet set = data.getDataSetByIndex(0);
            data.addEntry(new Entry(set.getEntryCount(), valueX), 0);
            data.addEntry(new Entry(set.getEntryCount(), valueY), 1);
            data.addEntry(new Entry(set.getEntryCount(), valueZ), 2);
            mChartView.notifyDataSetChanged();
            // limit the number of visible entries
            mChartView.setVisibleXRangeMaximum(100);
            // move to the latest entry
            mChartView.moveViewToX(data.getEntryCount());
        }
    }

    public void PrepareLineChart(LineChart mChart ) {
        mChart.getDescription().setEnabled(false);
        mChart.setTouchEnabled(false);
        mChart.setScaleEnabled(false);
        Legend l = mChart.getLegend();
        l.setTextSize(12f);
        l.setEnabled(true);

        mChart.setDrawGridBackground(false);
        mChart.animateY(1100);
        mChart.setDrawBorders(false);

        mChart.setHighlightPerDragEnabled(true);
        mChart.setDragEnabled(true);
        mChart.setHardwareAccelerationEnabled(true);

        ArrayList<ILineDataSet> sets = new ArrayList<>();
        sets.add(CreateLineDataSet(Color.YELLOW, "X"));
        sets.add(CreateLineDataSet(Color.GREEN , "Y"));
        sets.add(CreateLineDataSet(Color.MAGENTA , "Z"));

        LineData data = new LineData(sets);
        mChart.setData(data);

        XAxis xl = mChart.getXAxis();
        xl.setDrawGridLines(true);
        xl.setAvoidFirstLastClipping(true);
        xl.setEnabled(true);

        YAxis leftAxis = mChart.getAxisLeft();
        leftAxis.setDrawGridLines(false);
        leftAxis.setDrawGridLines(true);

        YAxis rightAxis = mChart.getAxisRight();;
        rightAxis.setEnabled(false);

        mChartView = mChart;
    }

}
