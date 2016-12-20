

var CONSTRAINT_TYPE_USER = "user";
var CONSTRAINT_TYPE_WIT_STAMP = "wit_stamp";

function get_object_name_constraints(type)
{
    var t = type || "";
    switch (t)
    {
        //These constraints appy to: user, witstamp
        //since usernames are used as wit stamps, they can't contain commas either
        case CONSTRAINT_TYPE_USER:
        case CONSTRAINT_TYPE_WIT_STAMP:
            {
                return {
                    min: 1,
                    max: 256,
                    invalids: '#%/?`~!$^&*()=[]{}\\|;:\'"<>,',
                    invalids_regex: /[#%\/\?`~!\$\^&\*\(\)=\[\]\{\}\\\|;:'"<>,]/g
                };
            }
        // These constraints currently apply to: repo, vcstamp, vctag, branch
        default:
            {
                return {
                    min: 1,
                    max: 256,
                    invalids: '#%/?`~!$^&*()=[]{}\\|;:\'"<>',
                    invalids_regex: /[#%\/\?`~!\$\^&\*\(\)=\[\]\{\}\\\|;:'"<>]/g
                };
            }

    }
  
}

// Validate a name against a set of constraints.
// Returns an empty string if the name is valid or an error message if it's not.
function validate_object_name(name, constraints)
{
    var invalids = name.match(constraints.invalids_regex);
    if (name.length < constraints.min)
    {
        return "must have at least " + constraints.min + (constraints.min == 1 ? " character." : " characters.");
    }
    else if (name.length >= constraints.max)
    {
        return "must be shorter than " + constraints.max + " characters.";
    }
    else if (invalids)
    {
        return "may not contain: " + $.unique(invalids).join(" ");
    } 
    else
    {
        return "";
    }
}

// Example usage:
/*error = validate_object_name(name, get_object_name_constraints())
if (error)
{ // empty strings are false, right?
	print error // or stick it in whichever div/span it belongs in
}*/
