
#ifndef __vivi_marshal_MARSHAL_H__
#define __vivi_marshal_MARSHAL_H__

#include	<glib-object.h>

G_BEGIN_DECLS

/* BOOLEAN:OBJECT (./vivi_marshal.list:1) */
extern void vivi_marshal_BOOLEAN__OBJECT (GClosure     *closure,
                                          GValue       *return_value,
                                          guint         n_param_values,
                                          const GValue *param_values,
                                          gpointer      invocation_hint,
                                          gpointer      marshal_data);

/* BOOLEAN:OBJECT,POINTER (./vivi_marshal.list:2) */
extern void vivi_marshal_BOOLEAN__OBJECT_POINTER (GClosure     *closure,
                                                  GValue       *return_value,
                                                  guint         n_param_values,
                                                  const GValue *param_values,
                                                  gpointer      invocation_hint,
                                                  gpointer      marshal_data);

/* BOOLEAN:OBJECT,STRING,POINTER (./vivi_marshal.list:3) */
extern void vivi_marshal_BOOLEAN__OBJECT_STRING_POINTER (GClosure     *closure,
                                                         GValue       *return_value,
                                                         guint         n_param_values,
                                                         const GValue *param_values,
                                                         gpointer      invocation_hint,
                                                         gpointer      marshal_data);

/* BOOLEAN:VOID (./vivi_marshal.list:4) */
extern void vivi_marshal_BOOLEAN__VOID (GClosure     *closure,
                                        GValue       *return_value,
                                        guint         n_param_values,
                                        const GValue *param_values,
                                        gpointer      invocation_hint,
                                        gpointer      marshal_data);

G_END_DECLS

#endif /* __vivi_marshal_MARSHAL_H__ */

