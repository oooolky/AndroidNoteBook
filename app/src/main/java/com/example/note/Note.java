package com.example.note;

public class Note {
    private long id;
    private String content;
    private String time;
    private int tag;

    public Note(){

    }
    public Note(String content,String time,int tag){
        this.content=content;
        this.time=time;
        this.tag=tag;
    }

    public int getTag() {
        return tag;
    }

    public long getId() {
        return id;
    }

    public String getContent() {
        return content;
    }

    public String getTime() {
        return time;
    }

    public void setContent(String content) {
        this.content = content;
    }

    public void setId(long id) {
        this.id = id;
    }

    public void setTag(int tag) {
        this.tag = tag;
    }

    public void setTime(String time) {
        this.time = time;
    }

    @Override
    public String toString() {
        return content+"\n"+time.substring(5,16)+" "+id;
    }
}
