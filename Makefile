OPT = -O

MODULES = ../Modules

INCLUDE = -I$(C_DEV_DIRECTORY)/Include -I$(MODULES)/Include -I/usr/local/include

include $(C_DEV_DIRECTORY)/Make/Makefile.include

roidis_to_display: roidis_to_display.o roidis_to_display.ln
	$(CC) $(LFLAGS) roidis_to_display.o $(LIBS) -o $@
	lint -u $(LINTFLAGS) roidis_to_display.ln \
                    $(C_DEV_DIRECTORY)/llib-lmni.ln

###########################################################################

dump_mesh: dump_mesh.o
	$(CC) $(LFLAGS) dump_mesh.o $(LIBS) $(MODULES)/libgeometry.a -o $@

lint_dump_mesh: dump_mesh.ln
	lint -u $(LINTFLAGS) dump_mesh.ln \
          $(C_DEV_DIRECTORY)/llib-lmni.ln $(MODULES)/lint/llib-lgeometry.ln

###########################################################################

make_edges: make_edges.o
	$(CC) $(LFLAGS) make_edges.o $(LIBS) -o $@

lint_make_edges: make_edges.ln
	lint -u $(LINTFLAGS) make_edges.ln $(C_DEV_DIRECTORY)/llib-lmni.ln

###########################################################################

make_edge_points: make_edge_points.o
	$(CC) $(LFLAGS) make_edge_points.o $(LIBS) -o $@

lint_make_edge_points: make_edge_points.ln
	lint -u $(LINTFLAGS) make_edge_points.ln $(C_DEV_DIRECTORY)/llib-lmni.ln

###########################################################################

make_gradient: make_gradient.o
	$(CC) $(LFLAGS) make_gradient.o $(LIBS) -o $@

lint_make_gradient: make_gradient.ln
	lint -u $(LINTFLAGS) make_gradient.ln $(C_DEV_DIRECTORY)/llib-lmni.ln

###########################################################################

make_gradient_arrows: make_gradient_arrows.o
	$(CC) $(LFLAGS) make_gradient_arrows.o $(LIBS) -o $@

lint_make_gradient_arrows: make_gradient_arrows.ln
	lint -u $(LINTFLAGS) make_gradient_arrows.ln $(C_DEV_DIRECTORY)/llib-lmni.ln

###########################################################################

zoom_gradient: zoom_gradient.o
	$(CC) $(LFLAGS) zoom_gradient.o $(LIBS) -o $@

lint_zoom_gradient: zoom_gradient.ln
	lint -u $(LINTFLAGS) zoom_gradient.ln $(C_DEV_DIRECTORY)/llib-lmni.ln

###########################################################################

flip_activity: flip_activity.o
	$(CC) $(LFLAGS) flip_activity.o $(LIBS) -o $@

lint_flip_activity: flip_activity.ln
	lint -u $(LINTFLAGS) flip_activity.ln $(C_DEV_DIRECTORY)/llib-lmni.ln

###########################################################################

create_sphere: create_sphere.o
	$(CC) $(LFLAGS) create_sphere.o \
                  $(MODULES)/libgeometry.a $(LIBS) -o $@

lint_create_sphere: create_sphere.ln
	lint -u $(LINTFLAGS) create_sphere.ln $(C_DEV_DIRECTORY)/llib-lmni.ln $(MODULES)/lint/llib-lgeometry.ln

###########################################################################

create_tetra: create_tetra.o
	$(CC) $(LFLAGS) create_tetra.o \
             $(MODULES)/libgeometry.a $(LIBS) -o $@

lint_create_tetra: create_tetra.ln
	lint -u $(LINTFLAGS) create_tetra.ln $(C_DEV_DIRECTORY)/llib-lmni.ln \
                    $(MODULES)/lint/llib-lgeometry.ln

###########################################################################

subdivide_polygons: read_write.o subdivide_polygons.o
	$(CC) $(LFLAGS) read_write.o subdivide_polygons.o \
                  $(MODULES)/libgeometry.a $(LIBS) -o $@

lint_subdivide_polygons: read_write.ln subdivide_polygons.ln
	lint -u $(LINTFLAGS) read_write.ln subdivide_polygons.ln $(C_DEV_DIRECTORY)/llib-lmni.ln $(MODULES)/lint/llib-lgeometry.ln

###########################################################################

half_polygons: read_write.o half_polygons.o
	$(CC) $(LFLAGS) read_write.o half_polygons.o \
                  $(MODULES)/libgeometry.a $(LIBS) -o $@

lint_half_polygons: read_write.ln half_polygons.ln
	lint -u $(LINTFLAGS) read_write.ln half_polygons.ln $(C_DEV_DIRECTORY)/llib-lmni.ln $(MODULES)/lint/llib-lgeometry.ln

###########################################################################

deform_surface: deform_surface.o
	$(CC) $(LFLAGS) deform_surface.o $(MODULES)/libdeform.a $(MODULES)/libgeometry.a $(LIBS) -o $@

lint_deform_surface: deform_surface.ln
	lint -u $(LINTFLAGS) deform_surface.ln \
          $(MODULES)/lint/llib-ldeform.ln $(MODULES)/lint/llib-lgeometry.ln \
          $(C_DEV_DIRECTORY)/llib-lmni.ln

###########################################################################

transform_objects: transform_objects.o
	$(CC) $(LFLAGS) transform_objects.o \
                  $(MODULES)/libgeometry.a $(LIBS) -o $@

lint_transform_objects: transform_objects.ln
	lint -u $(LINTFLAGS) transform_objects.ln $(C_DEV_DIRECTORY)/llib-lmni.ln $(MODULES)/lint/llib-lgeometry.ln

###########################################################################

rotate_objects: rotate_objects.o
	$(CC) $(LFLAGS) rotate_objects.o $(LIBS) -o $@

lint_rotate_objects: rotate_objects.ln
	lint -u $(LINTFLAGS) rotate_objects.ln $(C_DEV_DIRECTORY)/llib-lmni.ln

###########################################################################

transform_to_world: transform_to_world.o
	$(CC) $(LFLAGS) transform_to_world.o \
                  $(MODULES)/libgeometry.a $(LIBS) -o $@

lint_transform_to_world: transform_to_world.ln
	lint -u $(LINTFLAGS) transform_to_world.ln $(C_DEV_DIRECTORY)/llib-lmni.ln $(MODULES)/lint/llib-lgeometry.ln

###########################################################################

evaluate: evaluate.o
	$(CC) $(LFLAGS) evaluate.o \
                  $(MODULES)/libgeometry.a $(LIBS) -o $@

lint_evaluate: evaluate.ln
	lint -u $(LINTFLAGS) evaluate.ln $(C_DEV_DIRECTORY)/llib-lmni.ln $(MODULES)/lint/llib-lgeometry.ln

###########################################################################

voxelate_landmarks: voxelate_landmarks.o
	$(CC) $(LFLAGS) voxelate_landmarks.o $(LIBS) -o $@

lint_voxelate_landmarks: voxelate_landmarks.ln
	lint -u $(LINTFLAGS) voxelate_landmarks.ln \
            $(C_DEV_DIRECTORY)/llib-lmni.ln

###########################################################################

fit_line: fit_line.o
	$(CC) $(LFLAGS) fit_line.o $(LIBS) -o $@

lint_fit_line: fit_line.ln
	lint -u $(LINTFLAGS) fit_line.ln $(C_DEV_DIRECTORY)/llib-lmni.ln

###########################################################################

copy_colours: copy_colours.o
	$(CC) $(LFLAGS) copy_colours.o $(LIBS) -o $@

lint_copy_colours: copy_colours.ln
	lint -u $(LINTFLAGS) copy_colours.ln $(C_DEV_DIRECTORY)/llib-lmni.ln

###########################################################################

to_ascii: to_ascii.o
	$(CC) $(LFLAGS) to_ascii.o $(LIBS) -o $@

lint_to_ascii: to_ascii.ln
	lint -u $(LINTFLAGS) to_ascii.ln $(C_DEV_DIRECTORY)/llib-lmni.ln

###########################################################################

make_labels: make_labels.o
	$(CC) $(LFLAGS) make_labels.o $(LIBS) -o $@

lint_make_labels: make_labels.ln
	lint -u $(LINTFLAGS) make_labels.ln $(C_DEV_DIRECTORY)/llib-lmni.ln

###########################################################################

examine_polygons: examine_polygons.o
	$(CC) $(LFLAGS) examine_polygons.o $(LIBS) -o $@

lint_examine_polygons: examine_polygons.ln
	lint -u $(LINTFLAGS) examine_polygons.ln  \
                     $(C_DEV_DIRECTORY)/llib-lmni.ln


###########################################################################

create_landmark_volume: create_landmark_volume.o
	$(CC) $(LFLAGS) create_landmark_volume.o $(LIBS) -o $@

lint_create_landmark_volume: create_landmark_volume.ln
	lint -u $(LINTFLAGS) create_landmark_volume.ln  \
                     $(C_DEV_DIRECTORY)/llib-lmni.ln

###########################################################################

create_landmark_full_volume: create_landmark_full_volume.o
	$(CC) $(LFLAGS) create_landmark_full_volume.o $(LIBS) -o $@

lint_create_landmark_full_volume: create_landmark_full_volume.ln
	lint -u $(LINTFLAGS) create_landmark_full_volume.ln  \
                     $(C_DEV_DIRECTORY)/llib-lmni.ln

###########################################################################

colour_curvature: colour_curvature.o
	$(CC) $(LFLAGS) colour_curvature.o $(MODULES)/libgeometry.a $(MODULES)/libdeform.a $(LIBS) -o $@

lint_colour_curvature: colour_curvature.ln
	lint -u $(LINTFLAGS) colour_curvature.ln \
          $(MODULES)/lint/llib-ldeform.ln \
          $(MODULES)/lint/llib-lgeometry.ln \
          $(C_DEV_DIRECTORY)/llib-lmni.ln


###########################################################################

extract_points: extract_points.o
	$(CC) $(LFLAGS) extract_points.o $(LIBS) -o $@

lint_extract_points: extract_points.ln
	lint -u $(LINTFLAGS) extract_points.ln $(C_DEV_DIRECTORY)/llib-lmni.ln

###########################################################################

dilate: dilate.o
	$(CC) $(LFLAGS) dilate.o $(LIBS) -o $@

lint_dilate: dilate.ln
	lint -u $(LINTFLAGS) dilate.ln $(C_DEV_DIRECTORY)/llib-lmni.ln


###########################################################################

left_minus_right: left_minus_right.o
	$(CC) $(LFLAGS) left_minus_right.o $(LIBS) -o $@

lint_left_minus_right: left_minus_right.ln
	lint -u $(LINTFLAGS) left_minus_right.ln $(C_DEV_DIRECTORY)/llib-lmni.ln

###########################################################################

count_tags: count_tags.o
	$(CC) $(LFLAGS) count_tags.o $(LIBS) -o $@

lint_count_tags: count_tags.ln
	lint -u $(LINTFLAGS) count_tags.ln $(C_DEV_DIRECTORY)/llib-lmni.ln

###########################################################################

count_region: count_region.o
	$(CC) $(LFLAGS) count_region.o $(LIBS) -o $@

lint_count_region: count_region.ln
	lint -u $(LINTFLAGS) count_region.ln $(C_DEV_DIRECTORY)/llib-lmni.ln


###########################################################################

create_volume: create_volume.o
	$(CC) $(LFLAGS) create_volume.o $(LIBS) -o $@

lint_create_volume: create_volume.ln
	lint -u $(LINTFLAGS) create_volume.ln $(C_DEV_DIRECTORY)/llib-lmni.ln


###########################################################################

transform_tags: transform_tags.o
	$(CC) $(LFLAGS) transform_tags.o $(LIBS) -o $@

lint_transform_tags: transform_tags.ln
	lint -u $(LINTFLAGS) transform_tags.ln $(C_DEV_DIRECTORY)/llib-lmni.ln


###########################################################################

tag_statistics: tag_statistics.o
	$(CC) $(LFLAGS) tag_statistics.o $(MODULES)/libstatistics.a $(LIBS) \
               -o $@

lint_tag_statistics: tag_statistics.ln
	lint -u $(LINTFLAGS) tag_statistics.ln $(MODULES)/lint/llib-lstatistics.ln \
              $(C_DEV_DIRECTORY)/llib-lmni.ln

