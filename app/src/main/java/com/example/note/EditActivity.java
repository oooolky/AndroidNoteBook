package com.example.note;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.EditText;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;

import java.text.SimpleDateFormat;
import java.util.Date;

public class EditActivity extends BaseActivity {
    EditText et;
    public Intent intent = new Intent();
    private String old_content = "";
    private String old_time = "";
    private int old_tag = 1;
    private long id = 0;
    private int openMode = 0;
    private int tag = 1;
    private boolean tagChange=false;
    private Toolbar myToolbar;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.edit_layout);
        myToolbar=findViewById(R.id.my_Toolbar);
        setSupportActionBar(myToolbar);
        getSupportActionBar().setHomeButtonEnabled(true);
        getSupportActionBar().setDisplayHomeAsUpEnabled(true);//设置toolber取代action bar
        myToolbar.setNavigationOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent=new Intent();
                autoSetMessage(intent);
                intent.putExtra("content",et.getText().toString());
                intent.putExtra("time",dateToStr());
                setResult(RESULT_OK,intent);
                finish();
            }
        });
        et=findViewById(R.id.et);
        Intent getIntent=getIntent();
        openMode=getIntent.getIntExtra("mode",0);
        if (openMode == 3) {
            //打开已存在的note，将内容写入到已编辑的笔记中，实现继续编辑
            id = getIntent.getLongExtra("id", 0);
            old_content = getIntent.getStringExtra("content");
            old_time = getIntent.getStringExtra("time");
            old_tag = getIntent.getIntExtra("tag", 1);
            et.setText(old_content);//填充内容
            et.setSelection(old_content.length());//移动光标的位置（最后），方便再次书写
        }

    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.edit_menu,menu);
        return super.onCreateOptionsMenu(menu);
    }

    @Override
    public boolean onOptionsItemSelected(@NonNull MenuItem item) {
        switch (item.getItemId()){
            case R.id.delete:
                //显示对话框
                new AlertDialog.Builder(EditActivity.this)
                        .setMessage("是否删除").setPositiveButton(android.R.string.yes, new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                if(openMode==4){//new note
                                    intent.putExtra("mode",-1);
                                    setResult(RESULT_OK,intent);
                                }else{//existing note
                                    intent.putExtra("mode",2);
                                    intent.putExtra("id",id);
                                    setResult(RESULT_OK,intent);
                                }
                                finish();
                            }
                        }).setNegativeButton(android.R.string.no, new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                dialog.dismiss();
                            }
                        }).
                        create().show();
                break;
        }
        return super.onOptionsItemSelected(item);
    }

    public boolean onKeyDown(int keyCode, KeyEvent event){
        if(keyCode==KeyEvent.KEYCODE_HOME){
            return true;
        }
        else if(keyCode==KeyEvent.KEYCODE_BACK){

            Intent intent=new Intent();
            autoSetMessage(intent);
            intent.putExtra("content",et.getText().toString());
            intent.putExtra("time",dateToStr());
            setResult(RESULT_OK,intent);
            finish();
            return true;
        }
        return super.onKeyDown(keyCode,event);
    }
    public void autoSetMessage(Intent intent) {
        if (openMode == 4) {
            Log.d("input", "input11111");
            //判断笔记是否为空，若为空，则不新增笔记
            if (et.getText().toString().length() == 0) {
                intent.putExtra("mode", -1);
            } else {
                intent.putExtra("mode", 0);
                intent.putExtra("content", et.getText().toString());
                intent.putExtra("time", dateToStr());
                intent.putExtra("tag", tag);
            }
        } else {
            //判断笔记是否被修改，或者标签是否更换，否则不更新笔记
            if (et.getText().toString().equals(old_content) && !tagChange) {
                intent.putExtra("mode", -1);
            } else {
                intent.putExtra("mode", 1);
                intent.putExtra("content", et.getText().toString());
                intent.putExtra("time", dateToStr());
                intent.putExtra("id", id);
                intent.putExtra("tag", tag);
            }
        }
    }
    public String dateToStr() {
        Date date = new Date();
        SimpleDateFormat simpleDateFormat = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
        return simpleDateFormat.format(date);
    }
}
