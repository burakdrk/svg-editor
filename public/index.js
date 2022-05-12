jQuery(document).ready(function() {
    // Populate the dropdown
    $.ajax({
        type: 'GET',       
        dataType: 'json',      
        url: '/get_files',  
        success: function (data) {
            console.log(`Loaded ${data.length} files successfully.`);

            data.forEach(json => {
                $('#dropdown').append(`<button class="dropdown-item" type="button">${json.name}</button>`);
                $('#filelogtable tbody').append(
                    `<tr>
                        <td ><a href="${json.name}" download><img src="${json.name}" style="width:200px" ></a></td>
                        <td><a href="${json.name}" download>${json.name}</a></td>
                        <td>${json.size} KB</td>
                        <td>${json.numRect}</td>
                        <td>${json.numCirc}</td>
                        <td>${json.numPaths}</td>
                        <td>${json.numGroups}</td>
                    </tr>`
                );
            })

            //Check if no files in log
            if($('#filelogtable tbody').children().length === 0)    {
                $('#isEmptyTable').html('No files');
            }

            //Listener for dropdown buttons
            $('.dropdown-item').click(function() {
                const path = `./uploads/${$(this).html()}`;
                
                $.ajax({
                    type: 'GET',
                    dataType: 'json',
                    url: '/get_svg',
                    data: {
                        path: path
                    },
                    success: (data) => {
                        console.log(data);
                        if(data.err)    {
                            alert(data.msg);
                            return;
                        }
                        
                        //clean table
                        $('#editTitleButt').removeClass('invisible');
                        $('#editDescButt').removeClass('invisible');
                        $('#rectButt').removeClass('invisible');
                        $('#circButt').removeClass('invisible');
                        $('#scaleButt').removeClass('invisible');
                        $('#dropdown_comp_butt').removeClass('invisible');
                        $('.dropdown-item').removeClass('active');
                        $(this).addClass('active');
                        $('#dropdownMenuButton').html($(this).html());
                        $('#svgviewtable table').html('');
                        $('#dropdown_comp').html('');

                        $('#svgviewtable table').append(`<tr><td colspan="3"><img src="${$(this).html()}" style="width:800px"></td></tr>`);
                        $('#svgviewtable table').append(`<tr><th style="width: 30%;">Title</th><th colspan="2">Description</td></tr>`);
                        $('#svgviewtable table').append(`<tr><td style="white-space: pre-wrap;">${data.title}</td><td style="white-space: pre-wrap;" colspan="2">${data.desc}</td></tr>`);
                        $('#svgviewtable table').append(`<tr><th>Component</th><th style="width: 53%;">Summary</th><th style="width: 17%;">Other attributes</th></tr>`);
                        $('#dropdown_comp').append(`<button class="baban" data-toggle="modal" data-target="#attrModal" type="button">SVG</button>`);

                        let i = 1;
                        data.rect.forEach(e => {
                            $('#svgviewtable table').append(`
                            <tr>
                                <td>Rectangle ${i}</td>
                                <td>x = ${e.x}${e.units}, y = ${e.y}${e.units}, width = ${e.w}${e.units}, height = ${e.h}${e.units}</td>
                                <td>${e.numAttr}</td>
                            </tr>
                            `);
                            $('#dropdown_comp').append(`<button class="baban" data-toggle="modal" data-target="#attrModal" type="button">Rectangle ${i}</button>`);
                            i++;
                        });

                        i = 1;
                        data.circle.forEach(e => {
                            $('#svgviewtable table').append(`
                            <tr>
                                <td>Circle ${i}</td>
                                <td>cx = ${e.cx}${e.units}, cy = ${e.cy}${e.units}, r = ${e.r}${e.units}</td>
                                <td>${e.numAttr}</td>
                            </tr>
                            `);
                            $('#dropdown_comp').append(`<button class="baban" data-toggle="modal" data-target="#attrModal" type="button">Circle ${i}</button>`);
                            i++;
                        });

                        i = 1;
                        data.path.forEach(e => {
                            $('#svgviewtable table').append(`
                            <tr>
                                <td>Path ${i}</td>
                                <td style="white-space: pre-wrap;">d = ${e.d}</td>
                                <td>${e.numAttr}</td>
                            </tr>
                            `);
                            $('#dropdown_comp').append(`<button class="baban" data-toggle="modal" data-target="#attrModal" type="button">Path ${i}</button>`);
                            i++;
                        });

                        i = 1;
                        data.group.forEach(e => {
                            $('#svgviewtable table').append(`
                            <tr>
                                <td>Group ${i}</td>
                                <td>${e.children} child element</td>
                                <td>${e.numAttr}</td>
                            </tr>
                            `);
                            $('#dropdown_comp').append(`<button class="baban" data-toggle="modal" data-target="#attrModal" type="button">Group ${i}</button>`);
                            i++;
                        });

                        $('.baban').click(function(e) {
                            $('#zulu').html($(this).html());
                            const split = $(this).html().split(' ');
                            $('#stuff').html('');

                            if(split[0] === 'SVG')  {
                                data.attr.forEach(e => {
                                    $('#stuff').append(`
                                        <li class="list-group-item">Name = ${e.name} , Value = ${e.value}</li>
                                    `);
                                });
                            }
                            
                            if(split[0] !== 'SVG')  {
                                //For required ones
                                if(split[0] === 'Rectangle')    {
                                    $('#stuff').append(`
                                        <li class="list-group-item">Name = x , Value = ${data.rect[split[1]-1].x}</li>
                                        <li class="list-group-item">Name = y , Value = ${data.rect[split[1]-1].y}</li>
                                        <li class="list-group-item">Name = width , Value = ${data.rect[split[1]-1].w}</li>
                                        <li class="list-group-item">Name = height , Value = ${data.rect[split[1]-1].h}</li>
                                    `);
                                }else if(split[0] === 'Circle') {
                                    $('#stuff').append(`
                                        <li class="list-group-item">Name = cx , Value = ${data.circle[split[1]-1].cx}</li>
                                        <li class="list-group-item">Name = cy , Value = ${data.circle[split[1]-1].cy}</li>
                                        <li class="list-group-item">Name = r , Value = ${data.circle[split[1]-1].r}</li>
                                    `);
                                }else if(split[0] === 'Path')   {
                                    $('#stuff').append(`
                                        <li class="list-group-item">Name = d , Value = ${data.path[split[1]-1].d}</li>
                                    `);
                                }
                                //For the others
                                $.ajax({
                                    type: 'GET',
                                    dataType: 'json',
                                    url: '/get_attributes',
                                    data: {
                                        index: split[1]-1,
                                        type: split[0],
                                        path: path
                                    },
                                    success: (data) => {
                                        console.log(data);

                                        if(data.err)    {
                                            alert(data.msg);
                                            return;
                                        }

                                        data.forEach(e => {
                                            $('#stuff').append(`
                                                <li class="list-group-item">Name = ${e.name} , Value = ${e.value}</li>
                                            `);
                                        });

                                    },
                                    error: (error) => {
                                        console.log(error);
                                    }
                                });
                            }
                            

                            
                            
                        });

                    },
                    error: (error) => {
                        console.log(error);
                    }
                });

            });
        },
        error: function(error) {
            console.log(error); 
        }
    });





    $('#createForm').submit(function(e) {
        e.preventDefault();

        let ok = true;
        let name = $('#createForm_name').val();
        const title = $('#createForm_title').val();
        const desc = $('#createForm_desc').val();
        
        if(name.length === 0)    {
            ok=false;
            alert('You need to enter a file name!');
        } 
        if(title.length > 255)   {
            ok=false;
            alert('Title can be 255 characters at max!');
        } 
        if(desc.length > 255)    {
            ok=false;
            alert('Description can be 255 characters at max!');
        } 

        if(ok)  {
            name = name + '.svg';
            $.ajax({
                type: 'GET',
                dataType: 'json',
                url: '/create_file',
                data: {
                    name: name,
                    title: title,
                    desc: desc
                },
                success: (data) => {
                    console.log(data);
                    if(data.err) {
                        alert(data.msg);
                        return;
                    }
                    this.submit();
                },
                error: (error) => {
                    console.log(error.responseJSON);
                    alert(error.responseJSON.msg);
                }
            });

        }
    });


    //to edit titles
    $('#titleForm').submit(function(e) {
        e.preventDefault();

        const path = `./uploads/${$('#dropdownMenuButton').html()}`;

        let ok = true;
        const title = $('#titleForm_title').val();

        if(title.length > 255)   {
            ok=false;
            alert('Title can be 255 characters at max!');
        } 
        
        if(ok)  {
            $.ajax({
                type: 'GET',
                dataType: 'json',
                url: '/set_titledesc',
                data: {
                    path: path,
                    title: title,
                    mode: 'title'
                },
                success: (data) => {
                    console.log(data);
                    if(data.err) {
                        alert(data.msg);
                        return;
                    }
                    this.submit();
                },
                error: (error) => {
                    console.log(error);
                }
            });
        }
    });

    //to edit descriptions
    $('#descForm').submit(function(e) {
        e.preventDefault();

        const path = `./uploads/${$('#dropdownMenuButton').html()}`;

        let ok = true;
        const desc = $('#descForm_desc').val();

        if(desc.length > 255)    {
            ok=false;
            alert('Description can be 255 characters at max!');
        } 

        if(ok)  {
            $.ajax({
                type: 'GET',
                dataType: 'json',
                url: '/set_titledesc',
                data: {
                    path: path,
                    desc: desc,
                    mode: 'desc'
                },
                success: (data) => {
                    console.log(data);
                    if(data.err) {
                        alert(data.msg);
                        return;
                    }
                    this.submit();
                },
                error: (error) => {
                    console.log(error);
                }
            });
        }
    });

    $('#attrForm').submit(function(e) {
        e.preventDefault();
        const split = $('#zulu').html().split(' ');
        const path = `./uploads/${$('#dropdownMenuButton').html()}`;
        
        let ok = true;
        const name = $('#attrForm_name').val();
        const value = $('#attrForm_val').val();

        if(name.length < 1) {
            alert('Name cannot be empty!');
            ok = false;
        }


        if(ok)  {
            $.ajax({
                type: 'GET',
                dataType: 'json',
                url: '/set_attribute',
                data: {
                    path: path,
                    name: name,
                    value: value,
                    type: split[0],
                    index: split[1]-1
                },
                success: (data) => {
                    console.log(data);

                    if(data.err) {
                        alert(data.msg);
                        return;
                    }
                    this.submit();
                },
                error: (error) => {
                    console.log(error);
                }
            });
        }

    });

    $('#rectForm').submit(function(e) {
        e.preventDefault();

        const path = `./uploads/${$('#dropdownMenuButton').html()}`;

        let ok = true;
        const x = $('#rectForm_x').val();
        const y = $('#rectForm_y').val();
        const width = $('#rectForm_w').val();
        const height = $('#rectForm_h').val();
        const units = $('#rectForm_units').val();

        if(isNaN(x))   {
            ok=false;
            alert('x has to be a number');
        }

        if(isNaN(y))   {
            ok=false;
            alert('y has to be a number');
        }

        if(isNaN(width))   {
            ok=false;
            alert('Width has to be a number');
        }

        if(isNaN(height))   {
            ok=false;
            alert('Height has to be a number');
        }

        if(x.length < 1)    {
            ok=false;
            alert('You need to enter x!');
        }
        if(y.length < 1)    {
            ok=false;
            alert('You need to enter y!');
        }
        if(width.length < 1)    {
            ok=false;
            alert('You need to enter a width!');
        }
        if(height.length < 1)    {
            ok=false;
            alert('You need to enter a height!');
        }
        if(width < 0)    {
            ok=false;
            alert('Width has to be at least 0!');
        } 
        if(height < 0)    {
            ok=false;
            alert('Height has to be at least 0!');
        } 
        if(units.length > 49)    {
            ok=false;
            alert('Thats too many characters for units');
        } 

        if(ok)  {
            $.ajax({
                type: 'GET',
                dataType: 'json',
                url: '/add_shape',
                data: {
                    path: path,
                    x: x,
                    y: y,
                    w: width,
                    h: height,
                    units: units,
                    mode: 'rect'
                },
                success: (data) => {
                    console.log(data);
                    if(data.err) {
                        alert(data.msg);
                        return;
                    }
                    this.submit();
                },
                error: (error) => {
                    console.log(error);
                }
            });
        }
    });

    $('#circForm').submit(function(e) {
        e.preventDefault();

        const path = `./uploads/${$('#dropdownMenuButton').html()}`;

        let ok = true;
        const cx = $('#circForm_cx').val();
        const cy = $('#circForm_cy').val();
        const r = $('#circForm_r').val();
        const units = $('#circForm_units').val();

        if(isNaN(cx))   {
            ok=false;
            alert('cx has to be a number');
        }

        if(isNaN(cy))   {
            ok=false;
            alert('cy has to be a number');
        }

        if(isNaN(r))   {
            ok=false;
            alert('r has to be a number');
        }

        if(cx.length < 1)    {
            ok=false;
            alert('You need to enter cx!');
        }
        if(cy.length < 1)    {
            ok=false;
            alert('You need to enter cy!');
        }
        if(r.length < 1)    {
            ok=false;
            alert('You need to enter a radius!');
        }
        if(r < 0)    {
            ok=false;
            alert('Radius has to be at least 0!');
        }
        if(units.length > 49)    {
            ok=false;
            alert('Thats too many characters for units');
        } 

        if(ok)  {
            $.ajax({
                type: 'GET',
                dataType: 'json',
                url: '/add_shape',
                data: {
                    path: path,
                    cx: cx,
                    cy: cy,
                    r: r,
                    units: units,
                    mode: 'circ'
                },
                success: (data) => {
                    console.log(data);
                    if(data.err) {
                        alert(data.msg);
                        return;
                    }
                    this.submit();
                },
                error: (error) => {
                    console.log(error);
                }
            });
        }
    });


    $('#scaleForm').submit(function(e) {
        e.preventDefault();

        const path = `./uploads/${$('#dropdownMenuButton').html()}`;
        const type = $('#scaleForm_type option:selected').val();

        let ok = true;
        const factor = $('#scaleForm_factor').val();

        if(isNaN(factor))   {
            ok=false;
            alert('Factor has to be a number!');
        }

        if(factor.length < 1)    {
            ok=false;
            alert('You need to enter a factor!');
        }
        
        if(factor < 0)    {
            ok=false;
            alert('Factor has to be at least 0!');
        }
        

        if(ok)  {
            $.ajax({
                type: 'GET',
                dataType: 'json',
                url: '/scale_shapes',
                data: {
                    path: path,
                    factor: factor,
                    mode: type
                },
                success: (data) => {
                    console.log(data);
                    if(data.err) {
                        alert(data.msg);
                        return;
                    }
                    this.submit();
                },
                error: (error) => {
                    console.log(error);
                }
            });
        }
    });

    
    



});

/* Validate extension for file uploads */
const fileValidation = () => {
    const file = document.getElementById('upFile');
    const path = file.value;

    if(!path.endsWith('.svg')) {
        alert('Invalid file type!');
        file.value = '';
        return false;
    }
}
