package sonto.models.fields;

import sonto.exception.InvalidValueTypeException;
import sonto.exception.OutOfBoundFieldException;

public class CharField extends Field {
    public CharField(String name, int max_length, FieldAttribute[] attrs) {
        super(name, attrs);
        this.maxLength = max_length;
        // TODO Auto-generated constructor stub
    }

    public int maxLength = 255;
    
    public void validate(Object v) throws Exception {
        if (!(v instanceof String)) {
            throw new InvalidValueTypeException ();
        }
        
        if (this.maxLength < ((String)v).length()) {
            throw new OutOfBoundFieldException();
        }
    }
}
