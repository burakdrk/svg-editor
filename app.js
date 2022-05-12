'use strict'

// C library API
const ffi = require('ffi-napi');

// Express App (Routes)
const express = require("express");
const app     = express();
const path    = require("path");
const fileUpload = require('express-fileupload');

app.use(fileUpload());
app.use(express.static(path.join(__dirname+'/uploads')));

// Minimization
const fs = require('fs');
const JavaScriptObfuscator = require('javascript-obfuscator');

// Important, pass in port as in `npm run dev 1234`, do not change
const portNum = process.argv[2];

// ffi stuff
const parser = ffi.Library('./libsvgparser.so', {
  'JSONforFileLog': ['string', ['string']],
  'addFile': ['string', ['string','string']],
  'JSONforSVGPanel': ['string', ['string']],
  'getTitle': ['string', ['string']],
  'getDesc': ['string', ['string']],
  'setDesc': ['string', ['string','string']],
  'setTitle': ['string', ['string','string']],
  'shapeAttributes': ['string', ['string','string','string']],
  'setAttributeWrapper': ['string', ['string','string','string','string','string']],
  'addComponentWrapper': ['string', ['string','string','string']],
  'scaleShapes': ['string', ['string','string','string']]
});

// Send HTML at root, do not change
app.get('/',function(req,res){
  res.sendFile(path.join(__dirname+'/public/index.html'));
});

// Send Style, do not change
app.get('/style.css',function(req,res){
  //Feel free to change the contents of style.css to prettify your Web app
  res.sendFile(path.join(__dirname+'/public/style.css'));
});

// Send obfuscated JS, do not change
app.get('/index.js',function(req,res){
  fs.readFile(path.join(__dirname+'/public/index.js'), 'utf8', function(err, contents) {
    const minimizedContents = JavaScriptObfuscator.obfuscate(contents, {compact: true, controlFlowFlattening: true});
    res.contentType('application/javascript');
    res.send(minimizedContents._obfuscatedCode);
  });
});

//Respond to POST requests that upload files to uploads/ directory
app.post('/upload', function(req, res) {
  if(!req.files) {
    return res.status(400).send('No files were uploaded.');
  }
 
  let uploadFile = req.files.uploadFile;
 
  // Use the mv() method to place the file somewhere on your server
  uploadFile.mv('uploads/' + uploadFile.name, function(err) {
    if(err) {
      return res.status(500).send(err);
    }

    res.redirect('/');
  });
});

//Respond to GET requests for files in the uploads/ directory
app.get('/uploads/:name', function(req , res){
  fs.stat('uploads/' + req.params.name, function(err, stat) {
    if(err == null) {
      res.sendFile(path.join(__dirname+'/uploads/' + req.params.name));
    } else {
      console.log('Error in file downloading route: '+err);
      res.send('');
    }
  });
});

//******************** Your code goes here ******************** 

// Get the files in the /uploads directory and return them for the table and dropdown
app.get('/get_files', function(req , res){
  let filesArray = [];

  fs.readdir('uploads/', (err, files) => {
    if(err) console.log(err);

    files.forEach(file => {
      if(file.endsWith('.svg')) {
        const ret = parser.JSONforFileLog(`./uploads/${file}`);

        if(ret !== 'invalid') {
          let json = JSON.parse(ret);

          json.name = file;
          json.size = Math.round((fs.statSync(`./uploads/${file}`).size)/1024);
          filesArray.push(json);
        }
      }
    });

    res.send(filesArray);
  });
});

//Create an .svg file using the name, desc and title provided
app.get('/create_file', function(req , res){
  const name = req.query.name;
  const title = req.query.title;
  const desc = req.query.desc;

  if(fs.existsSync('uploads/'+name))  {
    return res.status(400).send({msg: 'File already exists on the server!',err:true});
  }

  const jsonStr = JSON.stringify({
    title:title,
    descr:desc
  });
  
  const temp = parser.addFile(jsonStr, `./uploads/${name}`);
  if(temp === 'invalid')  {
    return res.send({msg:'Invalid SVG!',err:true});
  }

  res.send({msg:'Successfully created file.',err:false});
});

//For the svg view panel
app.get('/get_svg', function(req , res){
  const path = req.query.path;

  if(!fs.existsSync(path))  return res.send({msg: 'This file does not exist on the server!',err:true});

  const str = parser.JSONforSVGPanel(path);
  if(str === 'invalid') return res.send({msg:'Invalid SVG!',err:true});

  let json = JSON.parse(str);
  json.desc = parser.getDesc(path).replace(/(\r\n|\n|\r)/gm, "");
  json.title = parser.getTitle(path).replace(/(\r\n|\n|\r)/gm, "");
  
  res.send(json);
});

app.get('/set_titledesc', function(req , res){
  const path = req.query.path;

  if(!fs.existsSync(path))  return res.send({msg: 'This file does not exist on the server!',err:true});

  if(req.query.mode === 'title') {
    const str = parser.setTitle(path,req.query.title);
    if(str === 'invalid') return res.send({msg:'Invalid SVG!',err:true});
  }
  else if(req.query.mode === 'desc')  {
    const str = parser.setDesc(path,req.query.desc);
    if(str === 'invalid') return res.send({msg:'Invalid SVG!',err:true});
  }
  
  res.send({msg:'Title/desc edited successfully.',err:false});
});

app.get('/get_attributes', function(req , res){
  const path = req.query.path;

  if(!fs.existsSync(path))  return res.send({msg: 'This file does not exist on the server!',err:true});

  const str = parser.shapeAttributes(path, req.query.type, req.query.index);
  if(str === 'invalid') return res.send({msg:'Invalid SVG!',err:true});
  
  res.send(JSON.parse(str));
});

app.get('/set_attribute', function(req , res){
  const path = req.query.path;

  if(!fs.existsSync(path))  return res.send({msg: 'This file does not exist on the server!',err:true});

  const str = parser.setAttributeWrapper(path,req.query.name,req.query.value,req.query.type,req.query.index);
  if(str === 'invalid') return res.send({msg:'SVG became invalid! Not saved',err:true});
  else if (str === 'failed') return res.send({msg:'Editing failed!',err:true});
  else if(str ==='invalif') return res.send({msg:'Invalid SVG!',err:true});
  
  res.send({msg:'Successfully saved changes.',err:false});
});

app.get('/add_shape', function(req , res){
  const path = req.query.path;

  if(!fs.existsSync(path))  return res.send({msg: 'This file does not exist on the server!',err:true});

  let json;
  if(req.query.mode === 'circ') {
    json = {
      cx: Number(req.query.cx),
      cy: Number(req.query.cy),
      r: Number(req.query.r),
      units: req.query.units
    };
  }else {
    json = {
      x: Number(req.query.x),
      y: Number(req.query.y),
      w: Number(req.query.w),
      h: Number(req.query.h),
      units: req.query.units
    };
  }
  const jsonStr = JSON.stringify(json);

  const str = parser.addComponentWrapper(path,jsonStr,req.query.mode);
  if(str === 'invalid') return res.send({msg:'SVG became invalid! Not saved',err:true});
  else if(str ==='invalif') return res.send({msg:'Invalid SVG!',err:true});
  
  res.send({msg:'Successfully added shape.',err:false});
});

app.get('/scale_shapes', function(req , res){
  const path = req.query.path;

  if(!fs.existsSync(path))  return res.send({msg: 'This file does not exist on the server!',err:true});

  const str = parser.scaleShapes(path,req.query.factor,req.query.mode);
  if(str === 'invalid') return res.send({msg:'SVG became invalid! Not saved',err:true});
  else if(str ==='invalif') return res.send({msg:'Invalid SVG!',err:true});
  
  res.send({msg:'Successfully scaled.',err:false});
});

app.listen(portNum);
console.log('Running app at localhost: ' + portNum);