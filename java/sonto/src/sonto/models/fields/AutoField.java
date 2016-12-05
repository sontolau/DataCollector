package sonto.models.fields;

import sonto.exception.InvalidValueTypeException;
import sonto.exception.NotAttributeFoundException;

public class AutoField extends Field {
    public int base = 1;
    public int step = 1;
    
    public AutoField(String name, int base ,int step, FieldAttribute[] attrs) throws NotAttributeFoundException {
        super(name, attrs);
        
        this.base = base;
        this.step = step;
        // TODO Auto-generated constructor stub
    }

    public synchronized int get() {
        int id =  this.base;
        this.base += this.step;
        return id;
    }
    
    @Override
    public void validate(Object v) throws InvalidValueTypeException {
        if (!(v instanceof Integer)) {
            throw new InvalidValueTypeException ();
        }
    }
}
