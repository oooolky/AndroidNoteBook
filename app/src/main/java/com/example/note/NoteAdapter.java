package com.example.note;

import android.content.Context;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.text.TextUtils;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.Filter;
import android.widget.Filterable;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.List;

public class NoteAdapter extends BaseAdapter implements Filterable {
    private Context mContext;
    private List<Note> backList;//用来备份原始数据
    private List<Note> noteList;//新数据
    private MyFilter myFilter;
    public NoteAdapter(Context mContext,List<Note> noteList){
        this.mContext=mContext;
        this.noteList=noteList;
        backList=noteList;
    }

    @Override
    public int getCount() {
        return noteList.size();
    }

    @Override
    public Object getItem(int position) {
        return noteList.get(position);
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        SharedPreferences sharedPreferences = PreferenceManager.getDefaultSharedPreferences(mContext);
        mContext.setTheme(R.style.DayTheme);
        //定义视图，获取组件
        View view = View.inflate(mContext, R.layout.note_layout,null);
        TextView tv_content = view.findViewById(R.id.tv_content);
        TextView tv_time = view.findViewById(R.id.tv_time);

        //获取文本内容，并赋值
        String allText = noteList.get(position).getContent();
        String time = noteList.get(position).getTime();
        tv_content.setText(allText);
        tv_time.setText(time);

        //保存笔记的主键
        view.setTag(noteList.get(position).getId());
        return view;
    }

    @Override
    public Filter getFilter() {
        if(myFilter==null){
            myFilter=new MyFilter();
        }
        return myFilter;
    }

    class MyFilter extends Filter{
        @Override
        protected FilterResults performFiltering(CharSequence constraint) {
            FilterResults result=new FilterResults();
            List<Note> list;
            if(TextUtils.isEmpty(constraint)){
                list=backList;
            }else {
                list=new ArrayList<>();
                for(Note note:backList){
                    if(note.getContent().contains(constraint)){
                        list.add(note);
                    }
                }
            }
            result.values=list;
            result.count=list.size();
            return result;
        }

        @Override
        protected void publishResults(CharSequence constraint, FilterResults results) {
            noteList=(List<Note>)results.values;
            if(results.count>0){
                notifyDataSetChanged();
            }else{
                notifyDataSetInvalidated();
            }
        }
    }
}
