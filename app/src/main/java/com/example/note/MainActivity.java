package com.example.note;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.net.wifi.p2p.WifiP2pManager;
import android.os.Bundle;

import com.google.android.material.floatingactionbutton.FloatingActionButton;
import com.google.android.material.snackbar.Snackbar;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Display;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;

import androidx.appcompat.widget.SearchView;
import androidx.appcompat.widget.Toolbar;
import androidx.core.view.LayoutInflaterCompat;
import androidx.navigation.NavController;
import androidx.navigation.Navigation;
import androidx.navigation.ui.AppBarConfiguration;
import androidx.navigation.ui.NavigationUI;

import com.example.note.databinding.ActivityMainBinding;

import android.view.Menu;
import android.view.MenuItem;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.ListView;
import android.widget.PopupWindow;
import android.widget.RelativeLayout;
import android.widget.TextView;


import java.util.ArrayList;
import java.util.List;

public class MainActivity extends BaseActivity implements AdapterView.OnItemClickListener {

    private Context context=this;
    private NoteDatabase dbHelper;
    private NoteAdapter adapter;
    //private PlanDatabase planDbHelper;
    private List<Note> noteList=new ArrayList<>();
    private Toolbar myToolbar;
    FloatingActionButton btn;
    final String TAG="tag";
     TextView tv;
    private ListView lv;
    //弹出菜单
    private PopupWindow popupWindow;
    private PopupWindow popupCover;

    private ViewGroup customView;
    private ViewGroup coverView;
    private LayoutInflater layoutInflater ;
    private RelativeLayout main;
    private WindowManager wm;
    private DisplayMetrics metrics;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        btn=(FloatingActionButton) findViewById(R.id.fab);

        lv = findViewById(R.id.lv);
        myToolbar=findViewById(R.id.myToolbar);
        adapter = new NoteAdapter(getApplicationContext(), noteList);
        refreshListView();
        lv.setAdapter(adapter);

        setSupportActionBar(myToolbar);
        getSupportActionBar().setHomeButtonEnabled(true);
        getSupportActionBar().setDisplayHomeAsUpEnabled(true);//设置toolber取代action bar
        initPopUpView();
        myToolbar.setNavigationIcon(R.drawable.ic_baseline_menu_24);
        myToolbar.setNavigationOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                showPupUpView();
            }
        });
        lv.setOnItemClickListener(this);
        //myToolbar.setNavigationIcon(R.drawable.ic_main_dehaze_24);//设置菜单栏图标
        btn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {

//                Log.d(TAG,"onClick:click");
                Intent intent=new Intent(MainActivity.this,EditActivity.class);
                intent.putExtra("mode",4);
                startActivityForResult(intent,0);
            }
        });

       }
       public void initPopUpView(){
            layoutInflater=(LayoutInflater)MainActivity.this.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
            customView=(ViewGroup) layoutInflater.inflate(R.layout.setting_layout,null);
            coverView=(ViewGroup)layoutInflater.inflate(R.layout.setting_cover,null);
            main=findViewById(R.id.main_layout);
            wm=getWindowManager();
            metrics=new DisplayMetrics();
            wm.getDefaultDisplay().getMetrics(metrics);

       }
       public void showPupUpView(){
        int width=metrics.widthPixels;
        int height=metrics.heightPixels;
        popupCover=new PopupWindow(coverView,width,height,false);//可对焦
        popupWindow=new PopupWindow(customView,(int)(width*0.7),height,true);
        //给与颜色
        popupWindow.setBackgroundDrawable(new ColorDrawable(Color.WHITE));
        //在主界面加载成功后显示弹出
           findViewById(R.id.main_layout).post(new Runnable() {
               @Override
               public void run() {
                   popupCover.showAtLocation(main, Gravity.NO_GRAVITY,0,0);
                   popupWindow.showAtLocation(main, Gravity.NO_GRAVITY,0,0);
                   coverView.setOnTouchListener(new View.OnTouchListener() {
                       @Override
                       public boolean onTouch(View v, MotionEvent event) {
                           popupWindow.dismiss();
                           return true;
                       }
                   });
                   popupWindow.setOnDismissListener(new PopupWindow.OnDismissListener() {
                       @Override
                       public void onDismiss() {
                           popupCover.dismiss();
                       }
                   });
               }
           });
       }

       //接受传入结果
    @Override
    protected void onActivityResult(int requestCode, int resultCode, @Nullable Intent data) {
        long note_Id;//笔记的id，主键
        //接收结果
        int returnMode = data.getExtras().getInt("mode", -1);
        String content = data.getExtras().getString("content");
        String time = data.getExtras().getString("time");
        int tag = data.getExtras().getInt("tag", 1);
        note_Id = data.getExtras().getLong("id", 0);
        if (returnMode == 1) {//修改笔记
            //将结果写入Note实体类
            Note newNote = new Note(content, time, tag);
            newNote.setId(note_Id);//需要通过id进行修改
            CRUD op = new CRUD(context);
            op.open();
            op.addNote(newNote);
            op.close();
        }else if (returnMode == 0) {//新增笔记
            //将结果写入Note实体类
            Note newNote = new Note(content, time, tag);
            CRUD op = new CRUD(context);
            op.open();
            op.addNote(newNote);
            op.close();
        }else if(returnMode==2){//删除笔记
            Note curNote=new Note();
            curNote.setId(note_Id);
            CRUD op=new CRUD(context);
            op.open();
            op.removeNote(curNote);
            op.close();
        }
        super.onActivityResult(requestCode, resultCode, data);

        refreshListView();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.main_menu,menu);
        MenuItem mSearch=menu.findItem(R.id.action_search);
        SearchView mSearchView=(SearchView)mSearch.getActionView();
        //搜索栏为空的提示词
        mSearchView.setQueryHint("Search");
        mSearchView.setOnQueryTextListener(new SearchView.OnQueryTextListener(){
            @Override
            public boolean onQueryTextChange(String newText) {
                adapter.getFilter().filter(newText);
                return false;
            }

            @Override
            public boolean onQueryTextSubmit(String query) {

                return false;
            }
        });
        return super.onCreateOptionsMenu(menu);
    }

    @Override
    public boolean onOptionsItemSelected(@NonNull MenuItem item) {
        switch (item.getItemId()){
            case R.id.menu_clear:
                new AlertDialog.Builder(MainActivity.this).setMessage("全部删除吗？")
                        .setPositiveButton(android.R.string.yes, new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                dbHelper=new NoteDatabase(context);
                                SQLiteDatabase db=dbHelper.getWritableDatabase();
                                db.delete("notes",null,null);
                                db.execSQL("update sqlite_sequence set seq=0 where name='notes'");
                                refreshListView();
                            }
                        }).setNegativeButton(android.R.string.no, new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                dialog.dismiss();
                            }
                        }).create().show();
                break;
        }
        return super.onOptionsItemSelected(item);
    }

    public void refreshListView(){
        CRUD op = new CRUD(context);
        op.open();
        //设置adapter
        if (noteList.size() > 0) {
            noteList.clear();
        }
        noteList.addAll(op.getAllNotes());

        op.close();
        adapter.notifyDataSetChanged();
    }

    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
        switch (parent.getId()){
            case R.id.lv:
        Note curNote = (Note) parent.getItemAtPosition(position);
        Intent intent = new Intent(MainActivity.this, EditActivity.class);
        intent.putExtra("content", curNote.getContent());
        intent.putExtra("id", curNote.getId());
        intent.putExtra("time", curNote.getTime());
        //修改笔记的mode设置为3，与新建笔记的mode值为4进行区分
        intent.putExtra("mode", 3);
        intent.putExtra("tag", curNote.getTag());
        startActivityForResult(intent, 1);
        break;}
    }
}