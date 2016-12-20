/*
Copyright 2010-2013 SourceGear, LLC

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

load("../js_test_lib/vscript_test_lib.js");

function num_keys(obj)
{
    var count = 0;
    for (var prop in obj)
    {
        count++;
    }
    return count;
}

function get_keys(obj)
{
    var count = 0;
    var a = [];
    for (var prop in obj)
    {
        a[count++] = prop;
    }
    return a;
}

function err_must_contain(err, s)
{
    testlib.ok(-1 != err.indexOf(s), s);
}

function verify(zs, where, cs, which, count)
{
    var ids = zs.query("item", ["recid"], where, null, 0, 0, cs[which]);
    var msg = "count " + where + " on " + which + " should be " + count;
    testlib.ok(count == ids.length, msg);
}

function st_zing_fts()
{
    this.test_full_text_search = function ()
    {
        var t =
        {
            "version": 1,
            "rectypes":
            {
                "item":
                {
                    "merge":
                    {
                        "merge_type": "field",
                        "auto":
                        [
                            {
                                "op": "most_recent"
                            }
                        ]
                    },
                    "fields":
                    {
                        "label":
                        {
                            "datatype": "string"
                        },
                        "text":
                        {
                            "datatype": "string",
                            "constraints":
                            {
                                "full_text_search": true
                            }
                        }
                    }
                }
            }
        };

        var reponame = sg.gid();
        new_zing_repo(reponame);

        var repo = sg.open_repo(reponame);
        var zs = new zingdb(repo, sg.dagnum.TESTING_DB);

        var ztx = zs.begin_tx();
        ztx.set_template(t);

        var rec = ztx.new_record("item");
        rec.label = "Solomon";
        rec.text = "Better to live on a corner of the roof than share a house with a quarrelsome wife.";

        var rec = ztx.new_record("item");
        rec.label = "Kate Hennings";
        rec.text = "Why don't you go back to your double-wide and fry something.";

        var rec = ztx.new_record("item");
        rec.label = "George W. Bush";
        rec.text = "I'll be long gone before some smart person ever figures out what happened inside this Oval Office.";

        var rec = ztx.new_record("item");
        rec.label = "Gandalf";
        rec.text = "For sixty years, the Ring lay quiet in Bilbo's keeping, prolonging his life, delaying old age. But no longer, Frodo. Evil is stirring in Mordor. The Ring has awoken. It's heard its Master's call.";

        var rec = ztx.new_record("item");
        rec.label = "Hans Gruber";
        rec.text = "I am an exceptional thief, Mrs. McClane. And since I'm moving up to kidnapping, you should be more polite.";

        var rec = ztx.new_record("item");
        rec.label = "Bill";
        rec.text = "Now remember, no sarcasm, no back talk. At least, not for the first year or so. You're gonna have to let him warm up to you. He hates Caucasians, despises Americans, and has nothing but contempt for women. So in your case... it might take a little while.";

        var rec = ztx.new_record("item");
        rec.label = "Mal";
        rec.text = "You know, it ain't altogether wise, sneaking up on a man when he's handling his weapon.";

        var rec = ztx.new_record("item");
        rec.label = "Red";
        rec.text = "I have no idea to this day what those two Italian ladies were singing about. Truth is, I don't want to know. Some things are best left unsaid. I'd like to think they were singing about something so beautiful, it can't be expressed in words, and makes your heart ache because of it. I tell you, those voices soared higher and farther than anybody in a gray place dares to dream. It was like some beautiful bird flapped into our drab little cage and made those walls dissolve away, and for the briefest of moments, every last man in Shawshank felt free.";

        var rec = ztx.new_record("item");
        rec.label = "Humphrey";
        rec.text = "All right, settle down. Settle down... Now, before I begin the lesson, will those of you who are playing in the match this afternoon move your clothes down onto the lower peg immediately after lunch, before you write your letter home, if you're not getting your hair cut, unless you've got a younger brother who is going out this weekend as the guest of another boy, in which case, collect his note before lunch, put it in your letter after you've had your hair cut, and make sure he moves your clothes down onto the lower peg for you. Now...";

        var rec = ztx.new_record("item");
        rec.label = "Basil Fawlty";
        rec.text = "No! Look, what's the matter with you all? It's perfectly simple: we have the fire drill when I ring the fire bell. That wasn't the fire bell!";

        var rec = ztx.new_record("item");
        rec.label = "Frasier";
        rec.text = "Roger, at Cornell University they have an incredible piece of scientific equipment known as the Tunneling Electron Microscope. Now, this microscope is so powerful that by firing electrons you can actually see images of the atom, the infinitesimally minute building blocks of our universe. Roger, if I were using that microscope right now, I still wouldn't be able to locate my interest in your problem.";

        var rec = ztx.new_record("item");
        rec.label = "Verbal Kint";
        rec.text = "Who is Keyser Soze? He is supposed to be Turkish. Some say his father was German. Nobody believed he was real. Nobody ever saw him or knew anybody that ever worked directly for him, but to hear Kobayashi tell it, anybody could have worked for Soze. You never knew. That was his power. The greatest trick the Devil ever pulled was convincing the world he didn't exist. And like that, poof. He's gone.";

        var rec = ztx.new_record("item");
        rec.label = "Winston Churchill";
        rec.text = "An appeaser is one who feeds a crocodile, hoping it will eat him last.";

        var rec = ztx.new_record("item");
        rec.label = "Khan";
        rec.text = "He tasks me. He tasks me and I shall have him! I'll chase him 'round the moons of Nibia and 'round the Antares Maelstrom and 'round Perdition's flames before I give him up!";

        var rec = ztx.new_record("item");
        rec.label = "Coach Boone";
        rec.text = "This is where they fought the battle of Gettysburg. Fifty thousand men died right here on this field, fighting the same fight that we are still fighting among ourselves today. This green field right here, painted red, bubblin' with the blood of young boys. Smoke and hot lead pouring right through their bodies. Listen to their souls, men. I killed my brother with malice in my heart. Hatred destroyed my family. You listen, and you take a lesson from the dead. If we don't come together right now on this hallowed ground, we too will be destroyed, just like they were. I don't care if you like each other of not, but you will respect each other. And maybe... I don't know, maybe we'll learn to play this game like men.";

        var rec = ztx.new_record("item");
        rec.label = "Forrest Gump";
        rec.text = "That day, for no particular reason, I decided to go for a little run. So I ran to the end of the road. And when I got there, I thought maybe I'd run to the end of town. And when I got there, I thought maybe I'd just run across Greenbow County. And I figured, since I run this far, maybe I'd just run across the great state of Alabama. And that's what I did. I ran clear across Alabama. For no particular reason I just kept on going. I ran clear to the ocean. And when I got there, I figured, since I'd gone this far, I might as well turn around, just keep on going. When I got to another ocean, I figured, since I'd gone this far, I might as well just turn back, keep right on going.";

        var rec = ztx.new_record("item");
        rec.label = "Joker";
        rec.text = "Do you want to know why I use a knife? Guns are too quick. You can't savor all the... little emotions. In... you see, in their last moments, people show you who they really are. So in a way, I know your friends better than you ever did. Would you like to know which of them were cowards?";

        var rec = ztx.new_record("item");
        rec.label = "Will";
        rec.text = "Why shouldn't I work for the N.S.A.? That's a tough one, but I'll take a shot. Say I'm working at N.S.A. Somebody puts a code on my desk, something nobody else can break. Maybe I take a shot at it and maybe I break it. And I'm real happy with myself, 'cause I did my job well. But maybe that code was the location of some rebel army in North Africa or the Middle East. Once they have that location, they bomb the village where the rebels were hiding and fifteen hundred people I never met, never had no problem with, get killed. Now the politicians are sayin', \"Oh, send in the Marines to secure the area\" 'cause they don't give a shit. It won't be their kid over there, gettin' shot. Just like it wasn't them when their number got called, 'cause they were pullin' a tour in the National Guard. It'll be some kid from Southie takin' shrapnel in the ass. And he comes back to find that the plant he used to work at got exported to the country he just got back from. And the guy who put the shrapnel in his ass got his old job, 'cause he'll work for fifteen cents a day and no bathroom breaks. Meanwhile, he realizes the only reason he was over there in the first place was so we could install a government that would sell us oil at a good price. And, of course, the oil companies used the skirmish over there to scare up domestic oil prices. A cute little ancillary benefit for them, but it ain't helping my buddy at two-fifty a gallon. And they're takin' their sweet time bringin' the oil back, of course, and maybe even took the liberty of hiring an alcoholic skipper who likes to drink martinis and fuckin' play slalom with the icebergs, and it ain't too long 'til he hits one, spills the oil and kills all the sea life in the North Atlantic. So now my buddy's out of work and he can't afford to drive, so he's got to walk to the fuckin' job interviews, which sucks 'cause the shrapnel in his ass is givin' him chronic hemorrhoids. And meanwhile he's starvin', 'cause every time he tries to get a bite to eat, the only blue plate special they're servin' is North Atlantic scrod with Quaker State. So what did I think? I'm holdin' out for somethin' better. I figure fuck it, while I'm at it why not just shoot my buddy, take his job, give it to his sworn enemy, hike up gas prices, bomb a village, club a baby seal, hit the hash pipe and join the National Guard? I could be elected president.";

        var rec = ztx.new_record("item");
        rec.label = "Tyler Durden";
        rec.text = "Welcome to Fight Club. The first rule of Fight Club is: you do not talk about Fight Club. The second rule of Fight Club is: you DO NOT talk about Fight Club! Third rule of Fight Club: if someone yells \"stop!\", goes limp, or taps out, the fight is over. Fourth rule: only two guys to a fight. Fifth rule: one fight at a time, fellas. Sixth rule: the fights are bare knuckle. No shirt, no shoes, no weapons. Seventh rule: fights will go on as long as they have to. And the eighth and final rule: if this is your first time at Fight Club, you have to fight.";

        var rec = ztx.new_record("item");
        rec.label = "Matthew Quigley";
        rec.text = "You know your weapons. It's a lever-action, breech loader. Usual barrel length's thirty inches. This one has an extra four. It's converted to use a special forty-five caliber, hundred and ten grain metal cartridge, with a five-hundred forty grain paper patch bullet. It's fitted with double set triggers, and a Vernier sight. It's marked up to twelve-hundred yards. This one shoots a mite further.";

        var rec = ztx.new_record("item");
        rec.label = "Abraham Lincoln";
        rec.text = "Four score and seven years ago our fathers brought forth on this continent, a new nation, conceived in Liberty, and dedicated to the proposition that all men are created equal.  Now we are engaged in a great civil war, testing whether that nation, or any nation so conceived and so dedicated, can long endure. We are met on a great battle-field of that war. We have come to dedicate a portion of that field, as a final resting place for those who here gave their lives that that nation might live. It is altogether fitting and proper that we should do this.  But, in a larger sense, we can not dedicate -- we can not consecrate -- we can not hallow -- this ground. The brave men, living and dead, who struggled here, have consecrated it, far above our poor power to add or detract. The world will little note, nor long remember what we say here, but it can never forget what they did here. It is for us the living, rather, to be dedicated here to the unfinished work which they who fought here have thus far so nobly advanced. It is rather for us to be here dedicated to the great task remaining before us -- that from these honored dead we take increased devotion to that cause for which they gave the last full measure of devotion -- that we here highly resolve that these dead shall not have died in vain -- that this nation, under God, shall have a new birth of freedom -- and that government of the people, by the people, for the people, shall not perish from the earth.";

        var rec = ztx.new_record("item");
        rec.label = "Agent Smith";
        rec.text = "I'd like to share a revelation that I've had during my time here. It came to me when I tried to classify your species and I realized that you're not actually mammals. Every mammal on this planet instinctively develops a natural equilibrium with the surrounding environment but you humans do not. You move to an area and you multiply and multiply until every natural resource is consumed and the only way you can survive is to spread to another area. There is another organism on this planet that follows the same pattern. Do you know what it is? A virus. Human beings are a disease, a cancer of this planet. You're a plague and we are the cure.";

        var rec = ztx.new_record("item");
        rec.label = "Poe";
        rec.text = "Once upon a midnight dreary, while I pondered weak and weary, Over many a quaint and curious volume of forgotten lore, While I nodded, nearly napping, suddenly there came a tapping, As of some one gently rapping, rapping at my chamber door.  `'Tis some visitor,' I muttered, `tapping at my chamber door - Only this, and nothing more.' Ah, distinctly I remember it was in the bleak December, And each separate dying ember wrought its ghost upon the floor.  Eagerly I wished the morrow; - vainly I had sought to borrow From my books surcease of sorrow - sorrow for the lost Lenore - For the rare and radiant maiden whom the angels named Lenore - Nameless here for evermore.  And the silken sad uncertain rustling of each purple curtain Thrilled me - filled me with fantastic terrors never felt before; So that now, to still the beating of my heart, I stood repeating `'Tis some visitor entreating entrance at my chamber door - Some late visitor entreating entrance at my chamber door; - This it is, and nothing more,' Presently my soul grew stronger; hesitating then no longer, `Sir,' said I, `or Madam, truly your forgiveness I implore; But the fact is I was napping, and so gently you came rapping, And so faintly you came tapping, tapping at my chamber door, That I scarce was sure I heard you' - here I opened wide the door; - Darkness there, and nothing more.  Deep into that darkness peering, long I stood there wondering, fearing, Doubting, dreaming dreams no mortal ever dared to dream before; But the silence was unbroken, and the darkness gave no token, And the only word there spoken was the whispered word, `Lenore!' This I whispered, and an echo murmured back the word, `Lenore!' Merely this and nothing more.  Back into the chamber turning, all my soul within me burning, Soon again I heard a tapping somewhat louder than before.  `Surely,' said I, `surely that is something at my window lattice; Let me see then, what thereat is, and this mystery explore - Let my heart be still a moment and this mystery explore; - 'Tis the wind and nothing more!' Open here I flung the shutter, when, with many a flirt and flutter, In there stepped a stately raven of the saintly days of yore.  Not the least obeisance made he; not a minute stopped or stayed he; But, with mien of lord or lady, perched above my chamber door - Perched upon a bust of Pallas just above my chamber door - Perched, and sat, and nothing more.  Then this ebony bird beguiling my sad fancy into smiling, By the grave and stern decorum of the countenance it wore, `Though thy crest be shorn and shaven, thou,' I said, `art sure no craven.  Ghastly grim and ancient raven wandering from the nightly shore - Tell me what thy lordly name is on the Night's Plutonian shore!' Quoth the raven, `Nevermore.' Much I marvelled this ungainly fowl to hear discourse so plainly, Though its answer little meaning - little relevancy bore; For we cannot help agreeing that no living human being Ever yet was blessed with seeing bird above his chamber door - Bird or beast above the sculptured bust above his chamber door, With such name as `Nevermore.' But the raven, sitting lonely on the placid bust, spoke only, That one word, as if his soul in that one word he did outpour.  Nothing further then he uttered - not a feather then he fluttered - Till I scarcely more than muttered `Other friends have flown before - On the morrow he will leave me, as my hopes have flown before.' Then the bird said, `Nevermore.' Startled at the stillness broken by reply so aptly spoken, `Doubtless,' said I, `what it utters is its only stock and store, Caught from some unhappy master whom unmerciful disaster Followed fast and followed faster till his songs one burden bore - Till the dirges of his hope that melancholy burden bore Of 'Never-nevermore.' But the raven still beguiling all my sad soul into smiling, Straight I wheeled a cushioned seat in front of bird and bust and door; Then, upon the velvet sinking, I betook myself to linking Fancy unto fancy, thinking what this ominous bird of yore - What this grim, ungainly, ghastly, gaunt, and ominous bird of yore Meant in croaking `Nevermore.' This I sat engaged in guessing, but no syllable expressing To the fowl whose fiery eyes now burned into my bosom's core; This and more I sat divining, with my head at ease reclining On the cushion's velvet lining that the lamp-light gloated o'er, But whose velvet violet lining with the lamp-light gloating o'er, She shall press, ah, nevermore!  Then, methought, the air grew denser, perfumed from an unseen censer Swung by Seraphim whose foot-falls tinkled on the tufted floor.  `Wretch,' I cried, `thy God hath lent thee - by these angels he has sent thee Respite - respite and nepenthe from thy memories of Lenore!  Quaff, oh quaff this kind nepenthe, and forget this lost Lenore!' Quoth the raven, `Nevermore.' `Prophet!' said I, `thing of evil! - prophet still, if bird or devil! - Whether tempter sent, or whether tempest tossed thee here ashore, Desolate yet all undaunted, on this desert land enchanted - On this home by horror haunted - tell me truly, I implore - Is there - is there balm in Gilead? - tell me - tell me, I implore!' Quoth the raven, `Nevermore.' `Prophet!' said I, `thing of evil! - prophet still, if bird or devil!  By that Heaven that bends above us - by that God we both adore - Tell this soul with sorrow laden if, within the distant Aidenn, It shall clasp a sainted maiden whom the angels named Lenore - Clasp a rare and radiant maiden, whom the angels named Lenore?' Quoth the raven, `Nevermore.' `Be that word our sign of parting, bird or fiend!' I shrieked upstarting - `Get thee back into the tempest and the Night's Plutonian shore!  Leave no black plume as a token of that lie thy soul hath spoken!  Leave my loneliness unbroken! - quit the bust above my door!  Take thy beak from out my heart, and take thy form from off my door!' Quoth the raven, `Nevermore.' And the raven, never flitting, still is sitting, still is sitting On the pallid bust of Pallas just above my chamber door; And his eyes have all the seeming of a demon's that is dreaming, And the lamp-light o'er him streaming throws his shadow on the floor; And my soul from out that shadow that lies floating on the floor Shall be lifted - nevermore!";

        var rec = ztx.new_record("item");
        rec.label = "Frank Burns";
        rec.text = "I'm sick of hearing about the wounded. What about all the thousands of wonderful guys who are fighting this war without any of the credit or the glory that always goes to those lucky few who just happen to get shot?";

        var rec = ztx.new_record("item");
        rec.label = "Hawkeye Pierce";
        rec.text = "You pour six jiggers of gin and drink it while staring at a picture of Lorenzo Schwartz, the inventor of vermouth.";

        var rec = ztx.new_record("item");
        rec.label = "Steven Hiller";
        rec.text = "Y'know, this was supposed to be my weekend off, but noooo. You got me out here draggin' your heavy ass through the burnin' desert with your dreadlocks stickin' out the back of my parachute. You gotta come down here with a attitude, actin' all big and bad... ";

        var rec = ztx.new_record("item");
        rec.label = "b7741 bug";
        rec.text = "source and box";

        ztx.commit();

        var res = zs.query("item", ["label"], "text #MATCH 'gone'");
        print(sg.to_json__pretty_print(res));
        testlib.ok(3 == res.length);

        var res = zs.query("item", ["label"], "text #MATCH 'when'", "label #ASC");
        print(sg.to_json__pretty_print(res));

        var res = zs.query("item", ["label"], "text #MATCH 'shore'");
        print(sg.to_json__pretty_print(res));
        testlib.ok(1 == res.length);

        var res = zs.query("item", ["label"], "text #MATCH 'office'");
        print(sg.to_json__pretty_print(res));
        testlib.ok(1 == res.length);
        testlib.ok(res[0].label == "George W. Bush");

        var res = zs.query("item", ["label"], "text #MATCH 'off*'");
        print(sg.to_json__pretty_print(res));

        var res = zs.query("item", ["label"], "text #MATCH 'back'");
        print(sg.to_json__pretty_print(res));
        testlib.ok(6 == res.length);

        var res = zs.query("item", ["label"], "text #MATCH '\"back talk\"'");
        print(sg.to_json__pretty_print(res));
        testlib.ok(1 == res.length);
        testlib.ok(res[0].label == "Bill");

        var res = zs.query("item", ["label"], "text #MATCH 'weapons'");
        print(sg.to_json__pretty_print(res));
        testlib.ok(2 == res.length);

        var res = zs.query("item", ["label"], "text #MATCH 'weapons NEAR rule'");
        print(sg.to_json__pretty_print(res));
        testlib.ok(1 == res.length);
        testlib.ok(res[0].label == "Tyler Durden");

        var res = zs.query__fts("item", "text", "weapons", ["label"]);
        print(sg.to_json__pretty_print(res));

        var res = zs.query__fts("item", "text", "the", ["label"]);
        print(sg.to_json__pretty_print(res));
        testlib.ok(20 == res.length);
        testlib.ok(res[2].label == "Abraham Lincoln");

        var res = zs.query__fts("item", "text", "the", ["label"], 1, 2);
        print(res.length);
        print(sg.to_json__pretty_print(res));
        testlib.ok(1 == res.length);
        testlib.ok(res[0].label == "Abraham Lincoln");

        var res = zs.query__fts("item", "text", "the", ["label", "history"]);
        print(res.length);
        print(sg.to_json__pretty_print(res));
        testlib.ok(20 == res.length);
        testlib.ok(res[2].label == "Abraham Lincoln");
        testlib.ok(res[2].history);

        var res = zs.query__fts("item", "text", "the", ["label", "history"], 1, 2);
        print(res.length);
        print(sg.to_json__pretty_print(res));
        testlib.ok(1 == res.length);
        testlib.ok(res[0].label == "Abraham Lincoln");

        //bug b4771
        /*var res = zs.query__fts("item", "text", "source AND text OR box", ["label"]);
        print(sg.to_json__pretty_print(res));      
        testlib.equal(0, res.length, "this shouldn't return results");*/

        repo.close();
    };

	this.testAddFts = function() 
	{
		var before = 
        {
            "version" : 1,
            "rectypes" :
            {
                "item" :
                {
                    "merge" :
                    {
                        "merge_type" : "field",
                        "auto" : 
                        [
                            {
                                "op" : "most_recent"
                            }
                        ]
                    },
                    "fields" :
                    {
                        "label" :
                        {
                            "datatype" : "string"
                        },
                        "text" :
                        {
                            "datatype" : "string"
                        }
                    }
                }
            }
        };
		
        var after =
        {
            "version" : 1,
            "rectypes" :
            {
                "item" :
                {
                    "merge" :
                    {
                        "merge_type" : "field",
                        "auto" : 
                        [
                            {
                                "op" : "most_recent"
                            }
                        ]
                    },
                    "fields" :
                    {
                        "label" :
                        {
                            "datatype" : "string"
                        },
                        "text" :
                        {
                            "datatype" : "string",
                            "constraints" :
                            {
                                "full_text_search" : true
                            }
                        }
                    }
                }
            }
        };

        var reponame = sg.gid();
        new_zing_repo(reponame);

		testlib.log("creating repo");
        var repo = sg.open_repo(reponame);
		var ztx = false;

		try
		{
			testlib.log("opening db");
			var db = new zingdb(repo, sg.dagnum.TESTING_DB);

			testlib.log("first tx");
			ztx = db.begin_tx();
			ztx.set_template(before);

			var rec = ztx.new_record("item");
			rec.label = "Bill";
			rec.text = "Now remember, no sarcasm, no back talk. At least, not for the first year or so. You're gonna have to let him warm up to you. He hates Caucasians, despises Americans, and has nothing but contempt for women. So in your case... it might take a little while.";

			rec = ztx.new_record("item");
			rec.label = "Mal";
			rec.text = "You know, it ain't altogether wise, sneaking up on a man when he's handling his weapon.";

			ztx.commit();
			ztx = null;

			var exceptionCaught = false;
			var res = null;

			try
			{
				res = db.query__fts("item", "text", "altogether", ["label"]);
			}
			catch (e)
			{
				exceptionCaught = true;
			}

			if (! testlib.ok(exceptionCaught, "exception expected on non-fts template"))
			{
				testlib.log(sg.to_json__pretty_print(res));
				return;
			}

			testlib.log("second tx");
			
			ztx = db.begin_tx();
			ztx.set_template(after);
			ztx.commit();
			ztx = null;

			res = db.query__fts("item", "text", "altogether", ["label"]);
			testlib.log(sg.to_json__pretty_print(res));
			testlib.equal(1, res.length, "result count");
			testlib.equal("Mal", res[0].label, "result label");
		}
		finally
		{
			if (ztx)
				ztx.abort();
			if (repo)
				repo.close();
		}
	};
    
}

