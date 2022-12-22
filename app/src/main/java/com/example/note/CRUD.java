package com.example.note;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;

import java.util.ArrayList;
import java.util.List;

public class CRUD {
    SQLiteOpenHelper dbHandler;
    SQLiteDatabase db;
    private static final String[] columns={
            NoteDatabase.ID,
            NoteDatabase.CONTENT,
            NoteDatabase.TIME,
            NoteDatabase.MODE
    };
    //新建database
    public CRUD(Context context){
        dbHandler=new NoteDatabase(context);
    }
    public void open(){
        db=dbHandler.getWritableDatabase();
    }
    public void close(){
        dbHandler.close();
    }

    //把note加入到database
    public Note addNote(Note note){
        //处理数据类
        //存入数据库
        ContentValues contentValues=new ContentValues();
        contentValues.put(NoteDatabase.CONTENT,note.getContent());
        contentValues.put(NoteDatabase.TIME,note.getTime());
        contentValues.put(NoteDatabase.MODE,note.getTag());
        //放入数据库后的id值，自增长
        long insertId=db.insert(NoteDatabase.TABLE_NAME,null,contentValues);
        note.setId(insertId);
        return note;
    }
    public Note getNote(long id){
        Cursor cursor=db.query(NoteDatabase.TABLE_NAME,columns,NoteDatabase.ID+"=?",
                new String[]{String.valueOf(id)},null,null,null,null);
        if(cursor!=null)cursor.moveToFirst();
        Note e=new Note(cursor.getString(1),cursor.getString(2),cursor.getInt(3));
        return e;
    }
    public List<Note> getAllNotes(){
        Cursor cursor=db.query(NoteDatabase.TABLE_NAME,columns,null,
                null,null,null,null,null);
        List<Note> notes=new ArrayList<>();
        //如果有数据的话
        if(cursor.getCount()>0){
            while(cursor.moveToNext()){
                Note note=new Note();
                note.setId(cursor.getLong(cursor.getColumnIndexOrThrow(NoteDatabase.ID)));
                note.setContent(cursor.getString(cursor.getColumnIndexOrThrow(NoteDatabase.CONTENT)));
                note.setTime(cursor.getString(cursor.getColumnIndexOrThrow(NoteDatabase.TIME)));
                note.setTag(cursor.getInt(cursor.getColumnIndexOrThrow(NoteDatabase.MODE)));
                notes.add(note);

            }
        }
        return notes;
    }
    public int updateNote(Note note){
        ContentValues values = new ContentValues();
        values.put(NoteDatabase.CONTENT,note.getContent());
        values.put(NoteDatabase.TIME,note.getTime());
        values.put(NoteDatabase.MODE,note.getTag());
        return db.update(NoteDatabase.TABLE_NAME,values,
                NoteDatabase.ID+"=?",new String[]{String.valueOf(note.getId())});
    }
    public void removeNote(Note note){
        db.delete(NoteDatabase.TABLE_NAME,NoteDatabase.ID+"="+note.getId(),null);
    }
}
